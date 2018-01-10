#! /usr/bin/env python
# -*- coding: utf-8 -*-

# LICENSE
#
#   This file is part of Universal Mega Dumper.
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

import os
import sys
import glob
import time
import serial
import getopt
import argparse
import struct

##### sudo apt install python3-pip
##### python3 -m pip install pyserial 

## https://docs.python.org/3/howto/argparse.html

##### working with bytes and string
#>>> b'\xde\xad\xbe\xef'.hex()
#'deadbeef'
#and reverse:
#>>> bytes.fromhex('deadbeef')
#b'\xde\xad\xbe\xef'

## open file, create if not there, append data
#with open("out.bin", "ab+") as f:
#   f.write(b'\xde\xad\xbe\xef')

## open file, create if not there, overwrite data
#with open("out.bin", "wb+") as f:
#   f.write(b'\xde\xad\xbe\xef')

####################################################################################
# class dbDumper
####################################################################################
class dbDumper:
    
    serialPort = ""
    mode = ""
    flashID = ""
    sfID = []
    sfFileList = {}
    sfSize = 0
    
    progressBarSize = 64
    dispBytesPerLine = 16
    
    readChunkSize = 4096
    
    ## max buffer size in dumper is 2048, all values here are for UBuntu 16.04
    burnChunkSize = 128             ## timeout at 256
                                    ## wrote 2MB at 128 in 168.948s
    
    sfReadChunkSize = 1024      ## read 2MB at 1024 in 29.119s
                                    ## read 2MB at 512 in 31.241s
    
    sfWriteChunkSize = 512      ## wrote 2MB at 1024 in 72.657s
                                    ## wrote 2MB at 512 in 35.431s
                                    ## wrote 2MB at 256 in 46.283s
    
    sfBurnChunkSize = 256           ## wrote 2MB at 256 in 89.661s
    
    sramWriteChunkSize = 128
                                    
    opTime = ""
    romInfo = {}
    
    checksumCalculated = 0
    checksumCart = 0
    
    
    ##############################################
    ## constructor
    ##############################################
    def __init__(self):
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
                ser = serial.Serial( port = serialport, baudrate = 115200, bytesize = serial.EIGHTBITS, parity = serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE, timeout=3)
                ser.write(bytes("flash\r\n","utf-8"))
                response = ser.readline().decode("utf-8")
                if response == "thunder\r\n":
                    dbPort = serialport
                ser.close()
            except (OSError, serial.SerialException):
                pass
                
        try:
            self.serialPort = serial.Serial( port = dbPort, baudrate = 115200, bytesize = serial.EIGHTBITS, parity = serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE, timeout=1)
        except (OSError, serial.SerialException):
            print("class dbDumper.__init__ - could not connect to dbDumper")
            sys.exit(1)
    
    ##############################################
    ## getFlashID
    ##############################################      
    def getFlashID(self, dataWidth):
        self.serialPort.write(bytes("getid\r\n","utf-8"))
        readBytes = self.serialPort.read(4)
        if( dataWidth == 16):
            self.flashID = int.from_bytes(readBytes, byteorder="big")
        else:
            self.flashID = int.from_bytes(readBytes, byteorder="little")

    ##############################################
    ## getSffId
    ##############################################      
    def getSfId(self):
        self.serialPort.write(bytes("sflid\r\n","utf-8"))
        readBytes = self.serialPort.read(5)
        self.sflID.clear()
        self.sflID.append(readBytes[0])
        self.sflID.append(readBytes[1])
        self.sflID.append(readBytes[2])
        self.sflID.append(readBytes[3])
        self.sflID.append(readBytes[4])

    ##############################################
    ## getSfFileList
    ##############################################      
    def getSfFileList(self):
        
        self.serialPort.write(bytes("sfsize\r\n","utf-8"))
        response = self.serialPort.readline().decode("utf-8")
        self.sfSize = int(response)
        
        self.serialPort.write(bytes("sflist\r\n","utf-8"))
        
        ## file list is comma delimited, ends in \r\n
        response = self.serialPort.readline().decode("utf-8")
        
        ## check if files were found
        if( response == "\r\n" ):
            print("no files found")
            self.sfFileList.clear()
        else:
            ## remove final comma and \r\n"
            response = response[:-3]
            
            files = response.split(",")
            self.sfFileList.clear()
            
            for item in files:
                fileInfo = item.split(";")
                self.sfFileList.update({fileInfo[0]: int(fileInfo[1])})
                
    ##############################################
    ## setMode
    ##############################################
    def setMode(self, mode):
        self.mode = mode
        self.serialPort.write(bytes("setmode {}\r\n".format(mode),"utf-8"))
        response = self.serialPort.readline().decode("utf-8")
        expect = "mode = {}\r\n".format(mode)
        if ( response != expect ):
            print("dbDumper.setMode - expected '{}' : received '{}'".format(expect, response))
    
    ##############################################
    ## program
    ##############################################
    def program(self, address, value, dataWidth):
        
        ## 8bits or 16bits wide?
        if (dataWidth == 8):
            progCmd = "prgbyte"
        else:
            progCmd = "prgword"
            
        cmd = "{0} {1} {2}\r\n".format(progCmd, address, value)
        self.serialPort.write(bytes(cmd,"utf-8"))

    ##############################################
    ## sferase
    ##############################################
    def sfEraseAll(self):
        
        startTime = time.time()
        self.serialPort.write(bytes("sferase w\r\n","utf-8"))
        
        response = self.serialPort.read(1).decode("utf-8")
        while( response == "." ):
            print(response, end="", flush=True)
            response = self.serialPort.read(1).decode("utf-8")
    
        self.opTime = time.time() - startTime
        print("")
    
    ##############################################
    ## sfReadFile
    ##############################################      
    def sfReadFile(self, sfFilename, outFile):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
        
        startTime = time.time()
        
        cmd = "sfread {0} {1}\r\n".format(sfFilename, self.sfReadChunkSize)
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        ## check if file exists
        response = self.serialPort.readline().decode("utf-8")
        if (response == "found\r\n"):
            response = self.serialPort.readline().decode("utf-8")
            fileSize = int(response)
            print("reading {0} bytes from {1}".format(fileSize, sfFilename))
            
            try:
                os.remove(outFile)
            except OSError:
                pass
            with open(outFile, "wb+") as f:
                pos = 0
                while pos < fileSize:
                    ## read chunkSize or less
                    if (fileSize - pos) > self.sfReadChunkSize: 
                        sizeOfRead = self.sfReadChunkSize
                    else:
                        sizeOfRead = (fileSize - pos)
                    
                    #send 1 byte to dumper to signal we are ready to receive this chunk                         
                    #self.serialPort.write(bytes("0","utf-8"))
                    response = self.serialPort.read(sizeOfRead)
                    f.write(response)
                    pos += sizeOfRead
                    cli_progress( (pos/fileSize) , self.progressBarSize )
            
        else:
            print("file {0} not found in serial flash".format(sfFilename))
        
        self.opTime = time.time() - startTime
    
    ##############################################
    ## sfBurnCart
    ##############################################
    def sfBurnCart(self, sfFilename):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
            
        startTime = time.time()
        pos = 0
    
        cmd = "sfburn {0} {1}\r\n".format(sfFilename, self.sflBurnChunkSize)
        #print("command = {0}".format(cmd), end="")
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        ## check if file exists
        response = self.serialPort.readline().decode("utf-8")
        if (response == "found\r\n"):
            response = self.serialPort.readline().decode("utf-8")
            fileSize = int(response)
            print("burning {0} bytes from {1} to the cartridge".format(fileSize, sfFilename))
            
            response = self.serialPort.readline().decode("utf-8")
            while (response != "done\r\n"):
                pos = int(response)
                cli_progress( (pos/fileSize) , self.progressBarSize )
                response = self.serialPort.readline().decode("utf-8")
        else:
            print("file {0} not found in serial flash".format(sfFilename))

        self.opTime = time.time() - startTime
                
    ##############################################
    ## sflwrite
    ##############################################
    def sflWriteFile(self, sfFilename, fileName):
        
        if ( len(sfFilename) > 12 ):
            print("{0} is longer than the maximum (8.3) 12 characters".format(sfFilename))
            return
            
        startTime = time.time()
        pos = 0
        fileSize = os.path.getsize(fileName)

        cmd = "sfwrite {0} {1} {2}\r\n".format(sfFilename, fileSize, self.sfWriteChunkSize)
        #print("command = {0}".format(cmd), end="")
        self.serialPort.write(bytes(cmd,"utf-8"))
        
        #response = self.serialPort.readline().decode("utf-8") 
        #print(response, end="")
        #response = self.serialPort.readline().decode("utf-8") 
        #print(response)
        
        with open(fileName, "rb") as f:
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
                cli_progress( (pos/fileSize) , self.progressBarSize )
        
        self.opTime = time.time() - startTime
    
    ##############################################
    ## eraseChip
    ##############################################
    def eraseChip(self, chip):
        
        startTime = time.time()
        self.serialPort.write(bytes("erase {0} w\r\n".format(chip),"utf-8"))
        
        response = self.serialPort.read(1).decode("utf-8")
        while( response == "." ):
            print(response, end="", flush=True)
            response = self.serialPort.read(1).decode("utf-8")
        
        self.opTime = time.time() - startTime
        print("")

    ##############################################
    ## readSMSROMHeader
    ##############################################
    def readSMSROMHeader(self):
        
        ## clear current rom info dictionnary
        self.romInfo.clear()
        
        ## check for valid header at 0x7FF0
        cmd = "rdbblk 0x7FF0 8\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Trademark": response })
        
        ## get checksum
        cmd = "rdbblk 0x7FFA 2\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(2)
        intVal = int.from_bytes(response, byteorder="little")
        self.romInfo.update({"Checksum": [ intVal, hex(intVal) ]})
        
        ## get product code
        cmd = "rdbblk 0x7FFC 3\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(3)
        intVal = int.from_bytes(response, byteorder="little") & 0x0FFFFF
        self.romInfo.update({"Product Code": [ intVal, hex(intVal) ]})
        
        ## get region and size
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

    ##############################################
    ## readGenesisROMHeader
    ##############################################
    def readGenesisROMHeader(self):
        
        ## clear current rom info dictionnary
        self.romInfo.clear()
        
        ## get console name
        cmd = "rdwblk 0x100 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Console Name": response })
        
        ## get copyright information
        cmd = "rdwblk 0x110 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Copyright": response })
        
        ## get domestic name
        cmd = "rdwblk 0x120 48\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(48).decode("utf-8", "replace")
        self.romInfo.update({"Domestic Name": response })
        
        ## get overseas name
        cmd = "rdwblk 0x150 48\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(48).decode("utf-8", "replace")
        self.romInfo.update({"Overseas Name": response })

        ## get serial number
        cmd = "rdwblk 0x180 14\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(14).decode("utf-8", "replace")
        self.romInfo.update({"Serial Number": response })

        ## get checksum
        cmd = "rdwblk 0x18E 2\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(2)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"Checksum": [ intVal, hex(intVal) ]})

        ## get io support
        cmd = "rdwblk 0x190 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"IO Support": response })
        
        ## get ROM Start Address
        cmd = "rdwblk 0x1A0 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"ROM Begin": [ intVal, hex(intVal) ]})

        ## get ROM End Address
        cmd = "rdwblk 0x1A4 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"ROM End": [ intVal, hex(intVal) ]})

        ## get Start of RAM
        cmd = "rdwblk 0x1A8 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"RAM Begin": [ intVal, hex(intVal) ]})

        ## get End of RAM
        cmd = "rdwblk 0x1AC 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"RAM End": [ intVal, hex(intVal) ]})
        
        ## get sram support
        cmd = "rdwblk 0x1B0 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        self.romInfo.update({"SRAM Support": response })
        
        ## get start of sram
        cmd = "rdwblk 0x1B4 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"SRAM Begin": [ intVal, hex(intVal) ]})
        
        ## get end of sram
        cmd = "rdwblk 0x1B8 4\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(4)
        intVal = int.from_bytes(response, byteorder="big")
        self.romInfo.update({"SRAM End": [ intVal, hex(intVal) ]})
        
        ## get modem support
        cmd = "rdwblk 0x1BC 12\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(12).decode("utf-8", "replace")
        self.romInfo.update({"Modem Support": response })
        
        ## get memo
        cmd = "rdwblk 0x1C8 40\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(40).decode("utf-8", "replace")
        self.romInfo.update({"Memo": response })
        
        ## get country support
        cmd = "rdwblk 0x1F0 16\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(16).decode("utf-8", "replace")
        self.romInfo.update({"Country Support": response })
        
    ##############################################
    ## read
    ##############################################
    def read(self, outfile, address, size, dataWidth, ram):
    
        endAddress = address + size
        startAddress = address
    
        ## read 8bits or 16bits wide?
        if (dataWidth == 8) :
            if not ram:
                readCmd = "rdbblk"
            else:
                readCmd = "rdsbblk"
        else:
            if not ram:
                readCmd = "rdwblk"
            else:
                readCmd = "rdswblk"

        startTime = time.time()
        
        ## read from dumper, output to console
        if outfile == "console":
            responsePtr = 0
            while address < endAddress:
                ## read chunkSize or less
                if (endAddress - address) > self.readChunkSize: 
                    sizeOfRead = self.readChunkSize
                else:
                    sizeOfRead = (endAddress - address)
                
                cmd = "{0} {1} {2}\r\n".format(readCmd, address, sizeOfRead)
                
                self.serialPort.write(bytes(cmd,"utf-8"))
                response = self.serialPort.read(sizeOfRead)
                
                ## loop through results, pretty display to console
                respCount = len(response)
                i = 0

                while i < respCount:
                    ## display bytesPerLine or less
                    if (respCount - i) > self.dispBytesPerLine:
                        bytesThisLine = self.dispBytesPerLine
                    else:
                        bytesThisLine = (respCount - i)
                    
                    ## get bytes for this line
                    line = []
                    for col in range(i, (i + bytesThisLine) ):
                        line.append(response[col])
                    
                    ## print address offset at start of line
                    print("{0:0{1}X}".format( (address+i), 6), end=": ")
                    
                    ## print hex values with space in between every byte
                    for c in line:
                        print("{0:0{1}X}".format(c,2), end = " ")

                    ## only print readable ascii chars, rest display as "."
                    for c in line:
                        if 32 <= c <= 126:
                            print("{0:s}".format(chr(c)), end = "")
                        else:
                            print(".", end = "")
                    print("")
                        
                    i += bytesThisLine
                
                address += sizeOfRead   
                
        ## output to file       
        else:
            try:
                os.remove(outfile)
            except OSError:
                pass
            with open(outFile, "wb+") as f:
                while address < endAddress:
                    ## read chunkSize or less
                    if (endAddress - address) > self.readChunkSize: 
                        sizeOfRead = self.readChunkSize
                    else:
                        sizeOfRead = (endAddress - address)
                    
                    cmd = "{0} {1} {2}\r\n".format(readCmd, address, sizeOfRead)
                                            
                    ## send command to Teensy, read response    
                    self.serialPort.write(bytes(cmd,"utf-8"))
                    response = self.serialPort.read(sizeOfRead)
                    f.write(response)
                    address += sizeOfRead
                    cli_progress( ((address - startAddress)/size) , self.progressBarSize )
                    
        self.opTime = time.time() - startTime
    
    ##############################################
    ## checksumGenesis
    ##############################################  
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
            ## read chunkSize or less
            if (endAddress - address) > self.readChunkSize: 
                sizeOfRead = self.readChunkSize
            else:
                sizeOfRead = (endAddress - address)
            
            ## must ignore header data
            
            cmd = "rdbblk {0} {1}\r\n".format(address, sizeOfRead)
        
            ## send command to Teensy, read response    
            self.serialPort.write(bytes(cmd,"utf-8"))
            response = self.serialPort.read(sizeOfRead)
        
            ## add up each word in response, limit to 16 bit width
            ## loop through results
            respCount = len(response)
            i = 0
            while i < respCount:
                thisWord = response[i],response[i + 1]
                intVal = int.from_bytes(thisWord, byteorder="little")
                self.checksumCalculated = (self.checksumCalculated + intVal) & 0xFFFF
                i += 2
            
            address += sizeOfRead
            cli_progress( (address/endAddress) , self.progressBarSize )
            
        self.opTime = time.time() - startTime
        
    ##############################################
    ## checksumGenesis
    ##############################################  
    def checksumGenesis(self):
        
        # read ROM header
        self.readGenesisROMHeader()
        
        # Genesis checksums start after the header
        address = 512
        startAddress = 512
        self.checksumCalculated = 0
        
        ## get integer value of checksum
        self.checksumCart = self.romInfo["Checksum"][0]
        endAddress = self.romInfo["ROM End"][0] + 1
        size = endAddress - address
        
        startTime = time.time()
        
        while address < endAddress:
            ## read chunkSize or less
            if (endAddress - address) > self.readChunkSize: 
                sizeOfRead = self.readChunkSize
            else:
                sizeOfRead = (endAddress - address)
            
            cmd = "rdwblk {0} {1}\r\n".format(address, sizeOfRead)
                                    
            ## send command to Teensy, read response    
            self.serialPort.write(bytes(cmd,"utf-8"))
            response = self.serialPort.read(sizeOfRead)
            
            ## add up each word in response, limit to 16 bit width
            ## loop through results
            respCount = len(response)
            i = 0
            while i < respCount:
                thisWord = response[i],response[i + 1]
                intVal = int.from_bytes(thisWord, byteorder="big")
                self.checksumCalculated = (self.checksumCalculated + intVal) & 0xFFFF
                i += 2
            
            address += sizeOfRead
            cli_progress( ((address - startAddress)/size) , self.progressBarSize )
            
        self.opTime = time.time() - startTime

    ##############################################
    ## saveWrite
    ##############################################
    def saveWrite(self, fileName, address, datawidth):      
        
        startTime = time.time()
        pos = 0
        fileSize = os.path.getsize(fileName)

        ## read 8bits or 16bits wide?
        if (dataWidth == 8) :
            progCmd = "wrsblk"
        else:
            progCmd = "wrsblk"
    
        with open(fileName, "rb") as f:
            while( pos < fileSize ):
                if( ( fileSize - pos ) >= self.sramWriteChunkSize):
                    sizeOfWrite = self.sramWriteChunkSize
                else:
                    sizeOfWrite = ( fileSize - pos )
                    
                line = f.read(sizeOfWrite)
                cmd = "{0} {1} {2}\r\n".format(progCmd, address, sizeOfWrite)   
                self.serialPort.write(bytes(cmd,"utf-8"))
                self.serialPort.write(line)
                
                response = self.serialPort.readline().decode("utf-8")
                #print(response, end="")
                
                pos += sizeOfWrite
                address += sizeOfWrite
                cli_progress( (pos/fileSize) , self.progressBarSize )
            
        self.opTime = time.time() - startTime

    ##############################################
    ## cdWrite
    ##############################################
    def cdWrite(self, fileName, address):       
        
        startTime = time.time()
        pos = 0
        fileSize = os.path.getsize(fileName)

        progCmd = "wrbrblk"
    
        with open(fileName, "rb") as f:
            while( pos < fileSize ):
                if( ( fileSize - pos ) >= self.sramWriteChunkSize):
                    sizeOfWrite = self.sramWriteChunkSize
                else:
                    sizeOfWrite = ( fileSize - pos )
                    
                line = f.read(sizeOfWrite)
                cmd = "{0} {1} {2}\r\n".format(progCmd, address, sizeOfWrite)   
                self.serialPort.write(bytes(cmd,"utf-8"))
                self.serialPort.write(line)
                
                response = self.serialPort.readline().decode("utf-8")
                #print(response, end="")
                
                pos += sizeOfWrite
                address += sizeOfWrite
                cli_progress( (pos/fileSize) , self.progressBarSize )
            
        self.opTime = time.time() - startTime

    ##############################################
    ## burn
    ##############################################
    def burn(self, fileName, address, dataWidth):
        
        pos = 0
        fileSize = os.path.getsize(fileName)
        
        ## read 8bits or 16bits wide?
        if (dataWidth == 8):
            progCmd = "prgbblk"
        else:
            progCmd = "prgwblk"
        
        startTime = time.time()
        
        with open(fileName, "rb") as f:
            while( pos < fileSize ):
                if( ( fileSize - pos ) >= self.burnChunkSize):
                    sizeOfWrite = self.burnChunkSize
                else:
                    sizeOfWrite = ( fileSize - pos )
                    
                line = f.read(sizeOfWrite)
                cmd = "{0} {1} {2}\r\n".format(progCmd, address, sizeOfWrite)   
                self.serialPort.write(bytes(cmd,"utf-8"))
                self.serialPort.write(line)
                
                response = self.serialPort.readline().decode("utf-8")
                #print(response, end="")
                
                pos += sizeOfWrite
                address += sizeOfWrite
                cli_progress( (pos/fileSize) , self.progressBarSize )
            
        self.opTime = time.time() - startTime

        
    #####
def printBinFileToConsole(fileName):
    
    chunkSize = 32
    
    fileSize = os.path.getsize(fileName)
    pos = 0
    with open(fileName, "rb") as f:
        while( pos < fileSize ):
            if( ( fileSize - pos ) >= chunkSize):
                line = f.read(chunkSize)
            else:
                line = f.read(fileSize - pos )
            
            ## print hex values with space in between every byte
            for c in line:
                print("{0:0{1}X}".format(c,2), end = " ")

            ## only print readable ascii chars
            for c in line:
                if 32 <= c <= 126:
                    print("{0:s}".format(chr(c)), end = "")
                else:
                    print(".", end = "")
            print("")
            
            pos += chunkSize

#####
def printProgressPercentage(percent):
    sys.stdout.write("\r%d%%" % percent)
    sys.stdout.flush()

def cli_progress(progress, bar_length=32):
    progress *= 100
    if progress < 100:
        percent = float( progress / 100 )
        hashes = "#" * int( round( percent * bar_length ) )
        spaces = ' ' * (bar_length - len(hashes))
        print("\rPercent: [{0}] {1:.3f}%".format(hashes + spaces, progress), end="", flush=True)
        #sys.stdout.write("\rPercent: [{0}] {1:.3f}%".format(hashes + spaces, progress))
        #sys.stdout.flush()
    else:
        hashes = "#" * bar_length
        print("\rPercent: [{0}] {1:.3f}%\r\n".format(hashes, progress), end="", flush=True)
        #sys.stdout.write("\rPercent: [{0}] {1:.3f}%\r\n".format(hashes, progress))
        #sys.stdout.flush()



####################################################################################
# MAIN
####################################################################################
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(prog="dbDumper 0.1.0.0")
    parser.add_argument("-m","--mode", help="Set the cartridge type", choices=["cv", "gn", "tg", "pc", "ms", "none"], type=str, required=True)
    
    readWriteArgs = parser.add_mutually_exclusive_group()
    readWriteArgs.add_argument("--getid", help="Get Flash ID from cartridge", action="store_true")
    readWriteArgs.add_argument("--sfid", help="Get Serial Flash ID from dumper", action="store_true")
    readWriteArgs.add_argument("--sferase", help="Erase Serial Flash", action="store_true")
    readWriteArgs.add_argument("--sflist", help="List Serial Flash Files", action="store_true")
    readWriteArgs.add_argument("--sfwrite", nargs=1, help="Write FILE to Serial Flash", type=str, metavar=('filename'))
    readWriteArgs.add_argument("--sfread", nargs=1, help="Read Serial Flash filename to FILE", type=str, metavar=('filename'))
    readWriteArgs.add_argument("--sfburn", nargs=1, help="Burn Serial Flash filename to cartridge", type=str, metavar=('filename'))
    readWriteArgs.add_argument("--checksum", help="Calculate ROM checksum", action="store_true")
    readWriteArgs.add_argument("--info", help="Read ROM header information", action="store_true")
    readWriteArgs.add_argument("--read", nargs=2, help="Read BYTECOUNT from ADDRESS, to FILE or console if not specified", type=str, metavar=('bytecount', 'address'))
    readWriteArgs.add_argument("--write", nargs=2, help="Write VALUE at ADDRESS", type=str, metavar=('value', 'address'))
    readWriteArgs.add_argument("--savewrite", nargs=1, help="Write FILE to SAVE MEMORY", type=str, metavar=('address'))
    readWriteArgs.add_argument("--cdwrite", nargs=1, help="Write FILE to CD MEMORY", type=str, metavar=('address'))
    readWriteArgs.add_argument("--program", nargs=2, help="Program VALUE at ADDRESS", type=str, metavar=('value', 'address'))
    readWriteArgs.add_argument("--burn", nargs=1, help="Burn FILE contents at ADDRESS", type=str, metavar=('address'))
    readWriteArgs.add_argument("--erase", nargs=1, help="Erase entire ROM contents", type=str, metavar=('chip'))
    readWriteArgs.add_argument("--swapbytes", nargs=1, help="Reverse the endianness of a file", type=str, metavar=('new file'))
    
    parser.add_argument("-f","--file", help="File path for read/write operations", default="console")
    parser.add_argument("-r","--ram", help="Target RAM/ROM memory on cartridge", action="store_true")
    
    args = parser.parse_args()
    
    ## figure out the select mode, and set it on the dbDumper
    mode = args.mode
    dbMode = ""         # single letter code for Teensy, just easier to switch/case single chars on Teensy
    console = ""        # human readable
    dataWidth = ""
    
    if mode == "cv":
        dbMode = "c"
        dumper = dbDumper()
        dumper.setMode(dbMode)
        console = "Colecovision"
        dataWidth = 8
    elif mode == "gn":
        dbMode = "g"
        dumper = dbDumper()
        dumper.setMode(dbMode)
        console = "Genesis"
        dataWidth = 16
    elif mode == "pc":
        dbMode = "p"
        dumper = dbDumper()
        dumper.setMode(dbMode)
        console = "PC Engine"
        dataWidth = 8
    elif mode == "tg":
        dbMode = "t"
        dumper = dbDumper()
        dumper.setMode(dbMode)
        console = "Turbografx"
        dataWidth = 8
    elif mode == "ms":
        dbMode = "m"
        dumper = dbDumper()
        dumper.setMode(dbMode)
        console = "Master System"
        dataWidth = 8
    elif mode == "none":
        console = "None"
        dumper = dbDumper()
    else:
        pass
    
    if args.getid:
        dumper.getFlashID(dataWidth)
        print("0x{0:0{1}X}".format(dumper.flashID,8))
        #print(dumper.flashID)
    
    elif args.sfid:
        dumper.getSfId()
        print (''.join('0x{:02X} '.format(x) for x in dumper.sflID))
        #print(dumper.sflID)
    
    elif args.sferase:
        print("erasing serial flash...")
        dumper.sfEraseAll()
        print("erase serial flash completed in {0:.3f} s".format(dumper.opTime))
    
    elif args.sfwrite:
        sfFilename = args.sfwrite[0]
        fileName = args.file
        if ( fileName == "console" ):
            print("must specify a --FILE to write to the serial flash")
        else:
            print("writing {0} to serial flash...".format(fileName))
            dumper.sfWriteFile(sflFilename, fileName)
            print("wrote {0} to serial flash as {1} in {2:.3f} s".format(fileName, sfFilename, dumper.opTime))
    
    elif args.sfburn:
        sfFilename = args.sfburn[0]
        dumper.sfBurnCart(sflFilename)
        print("burned {0} from serial flash to {1} in {2:.3f} s".format(sfFilename, console, dumper.opTime))
    
    elif args.sfread:
        sfFilename = args.sfread[0]
        fileName = args.file
        if ( fileName == "console" ):
            print("must specify a --FILE to write to the serial flash")
        else:
            dumper.sfReadFile(sfFilename, fileName)
            print("read {0} from serial flash to {1} in {2:.3f} s".format(sfFilename, fileName, dumper.opTime))
    
    elif args.sflist:
        dumper.getSfFileList()
        usedSpace = 0
        freeSpace = 0
        print("name           : bytes")
        print("--------------------------")
        for item in dumper.sfFileList.items():
            #print(item)
            print("{0}: {1}".format(item[0].ljust(15), item[1]))
            usedSpace += item[1]
            
        freeSpace = dumper.sfSize - usedSpace
        print("\n\r{0} of {1} bytes remaining".format(freeSpace, dumper.sfSize))
    
    elif args.info:
        if mode == "gn":
            dumper.readGenesisROMHeader()
        if mode == "ms":
            dumper.readSMSROMHeader()
            
        for item in sorted(dumper.romInfo.items()):
            print(item)
    
    elif args.erase:
        eraseChips = args.erase[0].split(",")
        for eraseChip in eraseChips:    
            print("erasing flash chip {0}...".format(eraseChip))
            dumper.eraseChip(eraseChip)
            print("erase flash chip {0} completed in {1:.3f} s".format(eraseChip, dumper.opTime))
    
    elif args.checksum:
        if mode == "gn":
            dumper.checksumGenesis()
            print("checksum completed in {0:.3f} s, calculated {1} expected {2}".format(dumper.opTime, dumper.checksumCalculated, dumper.checksumCart)) 
        if mode == "ms":
            dumper.checksumSMS()
            print("checksum completed in {0:.3f} s, calculated {1} expected {2}".format(dumper.opTime, dumper.checksumCalculated, dumper.checksumCart)) 
        
    elif args.read:
        outFile = args.file
        if( args.read[0].endswith("Kb") ):  
            byteCount = int(args.read[0][:-2], 0) * (2**7)
        elif( args.read[0].endswith("KB") ):
            byteCount = int(args.read[0][:-2], 0) * (2**10)
        elif( args.read[0].endswith("Mb") ):
            byteCount = int(args.read[0][:-2], 0) * (2**17)
        elif( args.read[0].endswith("MB") ):
            byteCount = int(args.read[0][:-2], 0) * (2**20)
        else:
            byteCount = int(args.read[0], 0)
        address = int(args.read[1], 0)
        print("reading {0} bytes starting at address 0x{1:X} from {2}".format(byteCount, address, console))
        dumper.read(outFile, address, byteCount, dataWidth, args.ram)
        print("read {0} bytes completed in {1:.3f} s".format(byteCount, dumper.opTime))
    
    elif args.program:
        value = int(args.program[0], 0)
        address = int(args.program[1], 0)
        print("programming value {0} at address 0x{1:X}".format(value, address))
        dumper.program(address, value, dataWidth)
        
    elif args.write:
        byteCount = args.write[0]
        address = args.write[1]
        print("writing value {0} at address 0x{1:X}".format(byteCount, address))
    
    elif args.savewrite:
        fileName = args.file
        address = int(args.savewrite[0], 0)
        dumper.saveWrite(fileName, address, dataWidth)
        print("wrote {0} to SRAM in {1:.3f} s".format(fileName, dumper.opTime))
    
    elif args.cdwrite:
        fileName = args.file
        address = int(args.cdwrite[0], 0)
        dumper.cdWrite(fileName, address)
        print("wrote {0} to CD RAM in {1:.3f} s".format(fileName, dumper.opTime))
        
    elif args.burn:
        fileName = args.file
        address = int(args.burn[0], 0)
        print("burning {0} contents to ROM at address 0x{1:X}".format(fileName, address))
        dumper.burn(fileName, address, dataWidth)
        print("burn {0} completed in {1:.3f} s".format(fileName, dumper.opTime))

    elif args.swapbytes:
        fileName = args.file
        outFile = args.swapbytes[0]
        fileSize = os.path.getsize(fileName)
        pos = 0
        
        try:
            os.remove(outFile)
        except OSError:
            pass
            
        with open(outFile, "wb+") as fwrite:
            with open(fileName, "rb") as fread:
                while(pos < fileSize):  
                    badEndian = fread.read(2)
                    revEndian = struct.pack('<h', *struct.unpack('>h', badEndian))
                    fwrite.write(revEndian)
                    pos += 2
        

    else:
        parser.print_help()
        pass

