#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file  classumd.py
# \author René Richard
# \brief This program allows to read and write to various game cartridges 
#        including: Genesis, Coleco, SMS, PCE - with possibility for 
#        future expansion.
######################################################################## 
# \copyright This file is part of Universal Mega Dumper.
#
#   Universal Mega Dumper is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Universal Mega Dumper is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Universal Mega Dumper.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################

import os
import sys
import glob
import time
import serial
import getopt
import argparse
import struct

## Universal Mega Dumper
#
#  All communications with the UMD are handled by the umd class, as well
#  as several ROM header read and checksum calculation routines.
class umd:
    
    ## UMD Modes
    cartType = ""
    modes = {"None" : 0,
            "Colecovision" : 1,
            "Genesis" : 2, 
            "SMS" : 3,
            "PCEngine" : 4,
            "Turbografx-16" : 5 }
    
    ## Console bus widths
    busWidth = {"None" : 0,
            "Colecovision" : 8,
            "Genesis" : 16, 
            "SMS" : 8,
            "PCEngine" : 8,
            "Turbografx-16" : 8 }
    
    writeBlockSize = {
            "sf"   : 512,
            "rom"  : 256,
            "save" : 128,
            "bram" : 128 }
    
    ## serial port object on which the UMD is found
    serialPort = ""
    flashID = ""
    sfID = []
    sfFileList = {}
    sfSize = 0
    
    progressBarSize = 64
    dispBytesPerLine = 16
    readChunkSize = 4096
    burnChunkSize = 128
    sfReadChunkSize = 1024
    sfWriteChunkSize = 512
    sfBurnChunkSize = 256
    sramWriteChunkSize = 128
                            
    opTime = ""
    romInfo = {}
    
    checksumCalculated = 0
    checksumCart = 0

########################################################################    
## The Constructor
#  \param self self
#
#  Windows/Linux agnostic, searches for the UMD
########################################################################
    def __init__(self, mode):        
        self.cartType = mode

########################################################################    
## connectUMD
#  \param self self
#  \param mode the UMD mode
#  
#  Retrieve the ROM's manufacturer flash ID
########################################################################
    def connectUMD(self, mode):

        #print("mode set value = {0}".format(self.modes.get(mode)))
        
        dbPort = ""
        # enumerate ports
        if sys.platform.startswith("win"):
            ports = ["COM%s" % (i + 1) for i in range(256)]
        elif sys.platform.startswith ("linux"):
            ports = glob.glob("/dev/tty[A-Za-z]*")
        else:
            raise EnvironmentError("Unsupported platform")

        # test for dumper on port
        for serialport in ports:
            try:
                ser = serial.Serial( port = serialport, baudrate = 460800, bytesize = serial.EIGHTBITS, parity = serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE, timeout=0.1)
                ser.write(bytes("flash\r\n","utf-8"))
                response = ser.readline().decode("utf-8")
                if response == "thunder\r\n":
                    dbPort = serialport
                ser.close()
            except (OSError, serial.SerialException):
                pass
        
        # use the port found above to setup a 'permanent' object to communicate with the UMD        
        try:
            self.serialPort = serial.Serial( port = dbPort, baudrate = 460800, bytesize = serial.EIGHTBITS, parity = serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE, timeout=1)
            # set the UMD mode while we're at it
            self.setMode(mode)
        except (OSError, serial.SerialException):
            print("class umd.__init__ - could not connect to umd")
            sys.exit(1)


########################################################################    
## getFlashID
#  \param self self
#  
#  Retrieve the ROM's manufacturer flash ID
########################################################################     
    def getFlashID(self, dataWidth):
        
        self.serialPort.write(bytes("getid\r\n","utf-8"))
        readBytes = self.serialPort.read(4)
        if( self.cartType == "Genesis" ):
            self.flashID = int.from_bytes(readBytes, byteorder="big")
        else:
            self.flashID = int.from_bytes(readBytes, byteorder="little")
                
########################################################################    
## getSfFileList
#  \param self self
#  \param mode the current type of cartridge
#  
#  Instruct the UMD to setup its ports for a particular cartridge type
######################################################################## 
    def setMode(self, mode):
        
        #self.modes.get(mode)
        
        self.serialPort.write(bytes("setmode {}\r\n".format(self.modes.get(mode)),"utf-8"))
        response = self.serialPort.readline().decode("utf-8")
        expect = "mode = {}\r\n".format(self.modes.get(mode))
        if ( response != expect ):
            print("umd.setMode - expected '{}' : received '{}'".format(expect, response))

########################################################################    
## sfGetId
#  \param self self
#  
#  Retrieve the serial flash manufacturer ID
########################################################################      
    def sfGetId(self):
        
        self.serialPort.write(bytes("sfid\r\n","utf-8"))
        readBytes = self.serialPort.read(5)
        self.sfID.clear()
        self.sfID.append(readBytes[0])
        self.sfID.append(readBytes[1])
        self.sfID.append(readBytes[2])
        self.sfID.append(readBytes[3])
        self.sfID.append(readBytes[4])

########################################################################    
## sfGetFileList
#  \param self self
#  
#  Get a list of files currently on the serial flash
########################################################################       
    def sfGetFileList(self):
        
        self.serialPort.write(bytes("sfsize\r\n","utf-8"))
        response = self.serialPort.readline().decode("utf-8")
        self.sfSize = int(response)
        
        self.serialPort.write(bytes("sflist\r\n","utf-8"))
        
        # file list is comma delimited, ends in \r\n
        response = self.serialPort.readline().decode("utf-8")
        
        # check if files were found
        if( response == "\r\n" ):
            print("no files found")
            self.sfFileList.clear()
        else:
            # remove final comma and \r\n"
            response = response[:-3]
            
            files = response.split(",")
            self.sfFileList.clear()
            
            for item in files:
                fileInfo = item.split(";")
                self.sfFileList.update({fileInfo[0]: int(fileInfo[1])})

########################################################################    
## sfEraseAll
#  \param self self
#  
#  Erase the entire contents of the UMD's serial flash
######################################################################## 
    def sfEraseAll(self):
        
        startTime = time.time()
        self.serialPort.write(bytes("sferase w\r\n","utf-8"))
        
        response = self.serialPort.read(1).decode("utf-8")
        while( response == "." ):
            print(response, end="", flush=True)
            response = self.serialPort.read(1).decode("utf-8")
    
        self.opTime = time.time() - startTime
        print("")
    
########################################################################    
## sfReadFile
#  \param self self
#  \param sfFileName the 8.3 name of the serial flash file
#  \param outfile the local file to write
#  
#  Read a file from the UMD's serial flash and write it to a local file
########################################################################   
    def sfReadFile(self, sfFilename, outfile):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
        
        startTime = time.time()
        
        cmd = "sfread {0} {1}\r\n".format(sfFilename, self.sfReadChunkSize)
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        # check if file exists
        response = self.serialPort.readline().decode("utf-8")
        if (response == "found\r\n"):
            response = self.serialPort.readline().decode("utf-8")
            fileSize = int(response)
            print("reading {0} bytes from {1}".format(fileSize, sfFilename))
            
            try:
                os.remove(outfile)
            except OSError:
                pass
            with open(outfile, "wb+") as f:
                pos = 0
                while pos < fileSize:
                    # read chunkSize or less
                    if (fileSize - pos) > self.sfReadChunkSize: 
                        sizeOfRead = self.sfReadChunkSize
                    else:
                        sizeOfRead = (fileSize - pos)
                    
                    # send 1 byte to dumper to signal we are ready to receive this chunk                         
                    # self.serialPort.write(bytes("0","utf-8"))
                    response = self.serialPort.read(sizeOfRead)
                    f.write(response)
                    pos += sizeOfRead
                    self.printProgress( (pos/fileSize) , self.progressBarSize )
            
        else:
            print("file {0} not found in serial flash".format(sfFilename))
        
        self.opTime = time.time() - startTime
    
########################################################################    
## sfBurnCart
#  \param self self
#  \param sfFileName the 8.3 name of the serial flash file
#  
#  Burn the cartridge ROM with the contents of a file from the UMD's 
#  serial flash
######################################################################## 
    def sfBurnCart(self, sfFilename):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
            
        startTime = time.time()
        pos = 0
    
        cmd = "sfburn {0} {1}\r\n".format(sfFilename, self.sfBurnChunkSize)
        # print("command = {0}".format(cmd), end="")
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        # check if file exists
        response = self.serialPort.readline().decode("utf-8")
        if (response == "found\r\n"):
            response = self.serialPort.readline().decode("utf-8")
            fileSize = int(response)
            print("burning {0} bytes from serial flash file {1} to the cartridge".format(fileSize, sfFilename))
            
            response = self.serialPort.readline().decode("utf-8")
            while (response != "done\r\n"):
                pos = int(response)
                self.printProgress( (pos/fileSize) , self.progressBarSize )
                response = self.serialPort.readline().decode("utf-8")
        else:
            print("file {0} not found in serial flash".format(sfFilename))

        self.opTime = time.time() - startTime
                
########################################################################    
## sfWriteFile
#  \param self self
#  \param sfFileName the 8.3 name of the serial flash file
#  \param filename the local file
#  
#  Write a file to the UMD's serial flash
########################################################################
    def sfWriteFile(self, sfFilename, filename):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
            
        startTime = time.time()
        pos = 0
        fileSize = os.path.getsize(filename)

        cmd = "sfwrite {0} {1} {2}\r\n".format(sfFilename, fileSize, self.sfWriteChunkSize)
        #print("command = {0}".format(cmd), end="")
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        #response = self.serialPort.readline().decode("utf-8") 
        #print(response, end="")
        #response = self.serialPort.readline().decode("utf-8") 
        #print(response)
        
        with open(filename, "rb") as f:
            while( pos < fileSize ):
                if( ( fileSize - pos ) >= self.sfWriteChunkSize):
                    sizeOfWrite = self.sfWriteChunkSize
                else:
                    sizeOfWrite = ( fileSize - pos )
                    
                line = f.read(sizeOfWrite)
                self.serialPort.write(line)
                
                #wait here while the flash is programmed
                response = self.serialPort.readline().decode("utf-8") 
                
                pos += sizeOfWrite
                self.printProgress( (pos/fileSize) , self.progressBarSize )
        
        self.opTime = time.time() - startTime
    
########################################################################    
## eraseChip
#  \param self self
#  \param chip the index of which cartridge chip to erase
#  
#  Erase the enitre contents of a flash on the cartridge. Some cartridges
#  like Genesis can have multiple flash ICs so an index must be specified.
########################################################################
    def eraseChip(self, chip):
        
        startTime = time.time()
        self.serialPort.write(bytes("erase {0} w\r\n".format(chip),"utf-8"))
        
        response = self.serialPort.read(1).decode("utf-8")
        while( response == "." ):
            print(response, end="", flush=True)
            response = self.serialPort.read(1).decode("utf-8")
        
        self.opTime = time.time() - startTime
        print("")

########################################################################    
## programSingle
#  \param self self
#  \param address the address at which to program
#  \param value the value to write
#  
#  Program a single byte/word into the cartridge
######################################################################## 
    def programSingle(self, address, value):
        
        # 8bits or 16bits wide?
        if cartType == "Genesis":
            progCmd = "prgword"
        else:
            progCmd = "prgbyte"
            
        cmd = "{0} {1} {2}\r\n".format(progCmd, address, value)
        self.serialPort.write(bytes(cmd,"utf-8"))

########################################################################    
## read
#  \param self self
#  \param address the address at which to start the read operation
#  \param size how many bytes to read
#  \param target what memory to read from the UMD
#  \param outfile the file in which to dump the binary data
#  
#  Read a succession of bytes from the cartridge, the output can be
#  displayed in the console window if "console" is specified in outfile
#  else a new file will be created/overwritten with the binary data
########################################################################
    def read(self, address, size, target, outfile):
    
        endAddress = address + size
        startAddress = address
        width = self.busWidth.get(self.cartType)
        
        #["rom", "save", "bram", "header"]
        if( width == 8 ):
            if target == "rom":
                readCmd = "rdbblk"
            elif target == "save":
                readCmd = "rdsbblk"
            elif target == "bram":
                readCmd = "rdbblk"
            else:
                pass
        elif( width == 16 ):
            if target == "rom":
                readCmd = "rdwblk"
            elif target == "save":
                readCmd = "rdswblk"
            elif target == "bram":
                readCmd = "rdwblk"
            else:
                pass
        else:    
            pass
        
        startTime = time.time()
        
        # read from UMD, output to console
        if outfile == "console":
            while address < endAddress:
                # read chunkSize or less
                if (endAddress - address) > self.readChunkSize: 
                    sizeOfRead = self.readChunkSize
                else:
                    sizeOfRead = (endAddress - address)
                
                cmd = "{0} {1} {2}\r\n".format(readCmd, address, sizeOfRead)
                
                self.serialPort.write(bytes(cmd,"utf-8"))
                response = self.serialPort.read(sizeOfRead)
                
                # loop through results, pretty display to console
                respCount = len(response)
                i = 0

                while i < respCount:
                    # display bytesPerLine or less
                    if (respCount - i) > self.dispBytesPerLine:
                        bytesThisLine = self.dispBytesPerLine
                    else:
                        bytesThisLine = (respCount - i)
                    
                    # get bytes for this line
                    line = []
                    for col in range(i, (i + bytesThisLine) ):
                        line.append(response[col])
                    
                    # print address offset at start of line
                    print("{0:0{1}X}".format( (address+i), 6), end=": ")
                    
                    # print hex values with space in between every byte
                    for c in line:
                        print("{0:0{1}X}".format(c,2), end = " ")

                    # only print readable ascii chars, rest display as "."
                    for c in line:
                        if 32 <= c <= 126:
                            print("{0:s}".format(chr(c)), end = "")
                        else:
                            print(".", end = "")
                    print("")
                        
                    i += bytesThisLine
                
                address += sizeOfRead   
                
        # output to file       
        else:
            try:
                os.remove(outfile)
            except OSError:
                pass
            with open(outfile, "wb+") as f:
                while address < endAddress:
                    # read chunkSize or less
                    if (endAddress - address) > self.readChunkSize: 
                        sizeOfRead = self.readChunkSize
                    else:
                        sizeOfRead = (endAddress - address)
                    
                    cmd = "{0} {1} {2}\r\n".format(readCmd, address, sizeOfRead)
                                            
                    # send command to Teensy, read response    
                    self.serialPort.write(bytes(cmd,"utf-8"))
                    response = self.serialPort.read(sizeOfRead)
                    f.write(response)
                    address += sizeOfRead
                    self.printProgress( ((address - startAddress)/size) , self.progressBarSize )
                    
        self.opTime = time.time() - startTime

########################################################################    
## write
#  \param self self
#  \param address the address at which to start the write
#  \param target what memory to write on the UMD
#  \param filename the local file to write
#  
#  Write the contents of a local file to the CD Backup RAM Cart.
# def read(self, address, size, width, target, outfile):
######################################################################## 
    def write(self, address, target, filename):       
        
        startTime = time.time()
        fileSize = os.path.getsize(filename)
        blockSize = self.writeBlockSize.get(target)
        width = self.busWidth.get(self.cartType)
        pos = 0
    
        #["rom", "save", "bram", "sf", "header"]
        if( width == 8 ):
            if target == "rom":
                writeCmd = "prgbblk"
            elif target == "save":
                writeCmd = "wrsblk"
            elif target == "bram":
                writeCmd = "wrbrblk"
            else:
                pass
        elif( width == 16 ):
            if target == "rom":
                writeCmd = "prgwblk"
            elif target == "save":
                writeCmd = "wrsblk"
            elif target == "bram":
                writeCmd = "wrbrblk"
            else:
                pass
        else:    
            pass
    
        with open(filename, "rb") as f:
            while( pos < fileSize ):
                if( ( fileSize - pos ) >= blockSize):
                    sizeOfWrite = blockSize
                else:
                    sizeOfWrite = ( fileSize - pos )
                    
                line = f.read(sizeOfWrite)
                cmd = "{0} {1} {2}\r\n".format(writeCmd, address, sizeOfWrite)   
                self.serialPort.write(bytes(cmd,"utf-8"))
                self.serialPort.write(line)
                
                # UMD writes "done\n\r" when complete"
                response = self.serialPort.readline().decode("utf-8")
                
                pos += sizeOfWrite
                address += sizeOfWrite
                self.printProgress( (pos/fileSize) , self.progressBarSize )
            
        self.opTime = time.time() - startTime

########################################################################    
## readSMSROMHeader
#  \param self self
#  
#  Read and format the ROM header for Sega Master System cartridge
########################################################################
    def readSMSROMHeader(self):
        
        # clear current rom info dictionnary
        self.romInfo.clear()
        
        # check for valid header at 0x7FF0
        cmd = "rdbblk 0x7FF0 8\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Trademark": response })
        
        # get checksum
        cmd = "rdbblk 0x7FFA 2\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(2)
        intVal = int.from_bytes(response, byteorder="little")
        self.romInfo.update({"Checksum": [ intVal, hex(intVal) ]})
        
        # get product code
        cmd = "rdbblk 0x7FFC 3\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(3)
        intVal = int.from_bytes(response, byteorder="little") & 0x0FFFFF
        self.romInfo.update({"Product Code": [ intVal, hex(intVal) ]})
        
        # get region and size
        cmd = "rdbblk 0x7FFF 1\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(1)
        intVal = int.from_bytes(response, byteorder="little")
        
        regionVal = (intVal & 0xF0) >> 4
        if regionVal == 3:
            regionStr = "SMS Japan"
        elif regionVal == 4:
            regionStr = "SMS Export"
        elif regionVal == 5:
            regionStr = "GG Japan"
        elif regionVal == 6:
            regionStr = "GG Export"
        elif regionVal == 7:
            regionStr = "GG Internation"
        else:
            regionStr = "unknown"
        
        self.romInfo.update({"Region": regionStr })
        
        romsizeVal = (intVal & 0x0F)
        if romsizeVal == 10:
            romsize = 8192
        elif romsizeVal == 11:
            romsize = 16384
        elif romsizeVal == 12:
            romsize = 32768
        elif romsizeVal == 13:
            romsize = 49152
        elif romsizeVal == 14:
            romsize = 65536
        elif romsizeVal == 15:
            romsize = 131072
        elif romsizeVal == 0:
            romsize = 262144
        elif romsizeVal == 1:
            romsize = 525288
        elif romsizeVal == 2:
            romsize = 1048576
        else:
            romsize = "unknown"
            
        self.romInfo.update({"Size": romsize }) 

########################################################################    
## readGenesisROMHeader
#  \param self self
#  
#  Read and format the ROM header for Sega Genesis cartridge
########################################################################
    def readGenesisROMHeader(self):
        
        # clear current rom info dictionnary
        self.romInfo.clear()
        
        # get console name
        cmd = "rdwblk 0x100 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Console Name": response })
        
        # get copyright information
        cmd = "rdwblk 0x110 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Copyright": response })
        
        # get domestic name
        cmd = "rdwblk 0x120 48\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(48).decode("utf-8", "replace")
        self.romInfo.update({"Domestic Name": response })
        
        # get overseas name
        cmd = "rdwblk 0x150 48\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(48).decode("utf-8", "replace")
        self.romInfo.update({"Overseas Name": response })

        # get serial number
        cmd = "rdwblk 0x180 14\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(14).decode("utf-8", "replace")
        self.romInfo.update({"Serial Number": response })

        # get checksum
        cmd = "rdwblk 0x18E 2\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(2)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"Checksum": [ intVal, hex(intVal) ]})

        # get io support
        cmd = "rdwblk 0x190 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"IO Support": response })
        
        # get ROM Start Address
        cmd = "rdwblk 0x1A0 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"ROM Begin": [ intVal, hex(intVal) ]})

        # get ROM End Address
        cmd = "rdwblk 0x1A4 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"ROM End": [ intVal, hex(intVal) ]})

        # get Start of RAM
        cmd = "rdwblk 0x1A8 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"RAM Begin": [ intVal, hex(intVal) ]})

        # get End of RAM
        cmd = "rdwblk 0x1AC 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"RAM End": [ intVal, hex(intVal) ]})
        
        # get sram support
        cmd = "rdwblk 0x1B0 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        self.romInfo.update({"SRAM Support": response })
        
        # get start of sram
        cmd = "rdwblk 0x1B4 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"SRAM Begin": [ intVal, hex(intVal) ]})
        
        # get end of sram
        cmd = "rdwblk 0x1B8 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"SRAM End": [ intVal, hex(intVal) ]})
        
        # get modem support
        cmd = "rdwblk 0x1BC 12\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(12).decode("utf-8", "replace")
        self.romInfo.update({"Modem Support": response })
        
        # get memo
        cmd = "rdwblk 0x1C8 40\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(40).decode("utf-8", "replace")
        self.romInfo.update({"Memo": response })
        
        # get country support
        cmd = "rdwblk 0x1F0 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Country Support": response })

########################################################################    
## checksumSMS
#  \param self self
#  
#  Compare the checksum in the SMS cartridge's header with a calculated
#  checksum.
######################################################################## 
    def checksumSMS(self):
        # read ROM header
        self.readSMSROMHeader()
        
        # SMS checksums skip the header data at between 0x7FF0 and 0x8000
        self.checksumCalculated = 0
        self.checksumCart = self.romInfo["Checksum"][0]
        address = 0
        endAddress = self.romInfo["Size"]
        
        startTime = time.time()
        
        while address < endAddress:
            # read chunkSize or less
            if (endAddress - address) > self.readChunkSize: 
                sizeOfRead = self.readChunkSize
            else:
                sizeOfRead = (endAddress - address)
            
            # must ignore header data
            
            cmd = "rdbblk {0} {1}\r\n".format(address, sizeOfRead)
        
            # send command to Teensy, read response    
            self.serialPort.write(bytes(cmd,"utf-8"))
            response = self.serialPort.read(sizeOfRead)
        
            # add up each word in response, limit to 16 bit width
            # loop through results
            respCount = len(response)
            i = 0
            while i < respCount:
                thisWord = response[i],response[i + 1]
                intVal = int.from_bytes(thisWord, byteorder="little")
                self.checksumCalculated = (self.checksumCalculated + intVal) & 0xFFFF
                i += 2
            
            address += sizeOfRead
            self.printProgress( (address/endAddress) , self.progressBarSize )
            
        self.opTime = time.time() - startTime
        
########################################################################    
## checksumGenesis
#  \param self self
#  
#  Compare the checksum in the Genesis cartridge's header with a calculated
#  checksum.
########################################################################  
    def checksumGenesis(self):
        
        # read ROM header
        self.readGenesisROMHeader()
        
        # Genesis checksums start after the header
        address = 512
        startAddress = 512
        self.checksumCalculated = 0
        
        # get integer value of checksum
        self.checksumCart = self.romInfo["Checksum"][0]
        endAddress = self.romInfo["ROM End"][0] + 1
        size = endAddress - address
        
        startTime = time.time()
        
        while address < endAddress:
            # read chunkSize or less
            if (endAddress - address) > self.readChunkSize: 
                sizeOfRead = self.readChunkSize
            else:
                sizeOfRead = (endAddress - address)
            
            cmd = "rdwblk {0} {1}\r\n".format(address, sizeOfRead)
                                    
            # send command to UMD, read response    
            self.serialPort.write(bytes(cmd,"utf-8"))
            response = self.serialPort.read(sizeOfRead)
            
            # add up each word in response, limit to 16 bit width
            # loop through results
            respCount = len(response)
            i = 0
            while i < respCount:
                thisWord = response[i],response[i + 1]
                intVal = int.from_bytes(thisWord, byteorder="big")
                self.checksumCalculated = (self.checksumCalculated + intVal) & 0xFFFF
                i += 2
            
            address += sizeOfRead
            self.printProgress( ((address - startAddress)/size) , self.progressBarSize )
            
        self.opTime = time.time() - startTime

########################################################################    
## printProgress
#  \param self self
#  \param progress the normalized progress 
#  \param barLength the full 100% length of the progress bar
#  
#  Display a lovely progress bar.
######################################################################## 
    def printProgress(self, progress, barLength=32):
        progress *= 100
        if progress < 100:
            percent = float( progress / 100 )
            hashes = "#" * int( round( percent * barLength ) )
            spaces = ' ' * (barLength - len(hashes))
            print("\rPercent: [{0}] {1:.3f}%".format(hashes + spaces, progress), end="", flush=True)
            #sys.stdout.write("\rPercent: [{0}] {1:.3f}%".format(hashes + spaces, progress))
            #sys.stdout.flush()
        else:
            hashes = "#" * barLength
            print("\rPercent: [{0}] {1:.3f}%\r\n".format(hashes, progress), end="", flush=True)
            #sys.stdout.write("\rPercent: [{0}] {1:.3f}%\r\n".format(hashes, progress))
            #sys.stdout.flush()
