#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file  hardware.py
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
#  All communications with the UMD are handled by the umd class
class umd:
    
    ## UMD Modes
    cartType = ""
    modes = {"None" : 0,
            "Colecovision" : 1,
            "Genesis" : 2, 
            "SMS" : 3,
            "PCEngine" : 4,
            "Turbografx-16" : 5,
            "Super Nintendo" : 6 }
    
    ## Console bus widths
    busWidth = {"None" : 0,
            "Colecovision" : 8,
            "Genesis" : 16, 
            "SMS" : 8,
            "PCEngine" : 8,
            "Turbografx-16" : 8,
            "Super Nintendo" : 8 }
    
    writeBlockSize = {
            "sf"   : 512,
            "rom"  : 256,
            "save" : 128,
            "bram" : 128 }
    
    ## serial port object on which the UMD is found
    serialPort = ""
    flashIDData = {}
    flashID = ""
    sfID = []
    sfFileList = {}
    sfSize = 0
    
    progressBarSize = 60 # 60 columns fits nicely in an 80 col terminal
    dispBytesPerLine = 16
    readChunkSize = 4096
    burnChunkSize = 128
    sfReadChunkSize = 1024
    sfWriteChunkSize = 512
    sfBurnChunkSize = 256
    sramWriteChunkSize = 128
    
    checksumRom = 0
    checksumCalc = 0
    romsize = 0
                   
    opTime = ""
    romInfo = {}

########################################################################    
## The Constructor
#  \param self self
#
#  Windows/Linux agnostic, searches for the UMD
########################################################################
    def __init__(self, mode, port):        
        self.cartType = mode
        self.connectUMD(mode, port)

########################################################################    
## connectUMD
#  \param self self
#  \param mode the UMD mode
#  
#  Retrieve the ROM's manufacturer flash ID
########################################################################
    def connectUMD(self, mode, port):

        #print("mode set value = {0}".format(self.modes.get(mode)))
        
        dbPort = ""
        
        if port is None:
            # enumerate ports
            if sys.platform.startswith("win"):
                ports = ["COM%s" % (i + 1) for i in range(256)]
            elif sys.platform.startswith ("linux"):
                ports = glob.glob("/dev/tty[A-Za-z]*")
            elif sys.platform.startswith ("darwin"):
                ports = glob.glob("/dev/cu*")
            else:
                raise EnvironmentError("Unsupported platform")
        else:
            ports = [port]

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
    def getFlashID(self):
        
        # query the chip
        self.serialPort.write(bytes("getid\r\n","utf-8"))
        # clear current info
        self.flashIDData.clear()
        # Manufacturer
        self.serialPort.write(bytes("getid m\r\n","utf-8"))
        data = int.from_bytes(self.serialPort.read(1), byteorder="little" )
        self.flashIDData.update({"Manufacturer": hex(data) })
        # Device
        self.serialPort.write(bytes("getid d\r\n","utf-8"))
        data = int.from_bytes(self.serialPort.read(1), byteorder="little" )
        self.flashIDData.update({"Device": hex(data) })
        # Type
        self.serialPort.write(bytes("getid t\r\n","utf-8"))
        data = int.from_bytes(self.serialPort.read(1), byteorder="little" )
        self.flashIDData.update({"Type": hex(data) })
        # Size
        self.serialPort.write(bytes("getid s\r\n","utf-8"))
        data = int.from_bytes(self.serialPort.read(4), byteorder="little" )
        self.flashIDData.update({"Size": hex(data) })
                
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
        self.serialPort.write(bytes("erase w\r\n".format(chip),"utf-8"))
        
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
    
        
        width = self.busWidth.get(self.cartType)
        
        #["rom", "save", "bram", "header", "fid", "sfid", "sf", "sflist", "byte", "word", "sbyte", "sword"]
        if( width == 8 ):
            if( target == "byte" or target == "sbyte" ):
                readCmd = "rdbyte"
            else:
                readCmd = "rdbblk"
        elif( width == 16 ):
            if( target == "word" or target == "sword" ):
                readCmd = "rdword"
                size = 2
            else:
                readCmd = "rdwblk"
        else:    
            pass
        
        endAddress = address + size
        startAddress = address
        startTime = time.time()
        
        # read from UMD, output to console
        if outfile == "console":
            while address < endAddress:
                # read chunkSize or less
                if (endAddress - address) > self.readChunkSize: 
                    sizeOfRead = self.readChunkSize
                else:
                    sizeOfRead = (endAddress - address)
                
                if( target == "byte" or target == "word" ):
                    cmd = "{0} {1}\r\n".format(readCmd, address)
                elif( target == "sbyte" or target == "sword" ):
                    cmd = "{0} {1} s\r\n".format(readCmd, address)
                elif( target == "save" ):
                    cmd = "{0} {1} {2} s\r\n".format(readCmd, address, sizeOfRead)
                # temporary hack for testing SF II on PCE
                elif( self.cartType == "PCEngine" and endAddress > 0x100000):
                    cmd = "{0} {1} {2} b\r\n".format(readCmd, address, sizeOfRead)
                else:
                    cmd = "{0} {1} {2}\r\n".format(readCmd, address, sizeOfRead)
                
                # show command
                print(cmd, end="")
                
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
                    
                    if( target == "byte" or target == "word" ):
                        cmd = "{0} {1}\r\n".format(readCmd, address)
                    elif( target == "sbyte" or target == "sword" ):
                        cmd = "{0} {1} s\r\n".format(readCmd, address)
                    elif( target == "save" ):
                        cmd = "{0} {1} {2} s\r\n".format(readCmd, address, sizeOfRead)
                    # temporary hack for testing SF II on PCE
                    elif( self.cartType == "PCEngine" and endAddress > 0x100000):
                        cmd = "{0} {1} {2} b\r\n".format(readCmd, address, sizeOfRead)
                    else:
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
## getRomSize(self):
#  \param self self
#  Ask the cart for the rom size.
########################################################################
    def getRomSize(self):
        cmd = "romsize\r\n"
        self.serialPort.write(bytes(cmd, "utf-8"))

        response = self.serialPort.readline().decode("utf-8")
        try:
            self.romsize = int(response)
        except:
            pass

########################################################################    
## checksumUMD(self):
#  \param self self
#
########################################################################
    def checksum(self):
        
        # Instruct the UMD to caculate the checksum of the ROM
        startTime = time.time()
        cmd = "checksum\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        response = self.serialPort.readline().decode("utf-8")
        self.romsize = int(response)
        print("checksum on 0x{0:X} bytes".format(self.romsize))
        
        response = self.serialPort.read(1).decode("utf-8")
        while( response == "." ):
            print(response, end="", flush=True)
            response = self.serialPort.read(1).decode("utf-8")
        
        
        response = self.serialPort.readline().decode("utf-8")
        try:
            self.checksumCalc = int(response)
        except:
            pass
        response = self.serialPort.readline().decode("utf-8")
        try:
            self.checksumRom = int(response)
        except:
            pass
        
        self.opTime = time.time() - startTime
        print("")


########################################################################    
## verify(self):
#  \param self self
#
########################################################################
    def verify(self, sfFilename):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
        
        startTime = time.time()
        
        cmd = "sfverify {0} {1}\r\n".format(sfFilename, self.sfReadChunkSize)
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        # check if file exists
        response = self.serialPort.readline().decode("utf-8")
        if (response == "found\r\n"):
            response = self.serialPort.readline().decode("utf-8")
            fileSize = int(response)
            print("verifying {0} bytes against {1}".format(fileSize, sfFilename))

            done = 0
            while( done == 0 ):
                response = self.serialPort.read(1).decode("utf-8")
                if( response == "$" ):
                    response = self.serialPort.readline().decode("utf-8")
                    address = int(response)
                    response = self.serialPort.readline().decode("utf-8")
                    expected = int(response)
                    response = self.serialPort.readline().decode("utf-8")
                    error = int(response)
                    print("error at 0x{0:X} expected 0x{1:X} read 0x{2:X}".format(address, expected, error))
                elif( response == "." ):
                    done = 0
                    #print(response, end="", flush=True)
                elif( response == "!" ):
                    done = 1
        else:
            print("file {0} not found in serial flash".format(sfFilename))

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
            print("\rPercent: [{0}] {1:7.3f}%".format(hashes + spaces, progress), end="", flush=True)
            #sys.stdout.write("\rPercent: [{0}] {1:.3f}%".format(hashes + spaces, progress))
            #sys.stdout.flush()
        else:
            hashes = "#" * barLength
            print("\rPercent: [{0}] {1:7.3f}%\r\n".format(hashes, progress), end="", flush=True)
            #sys.stdout.write("\rPercent: [{0}] {1:.3f}%\r\n".format(hashes, progress))
            #sys.stdout.flush()

