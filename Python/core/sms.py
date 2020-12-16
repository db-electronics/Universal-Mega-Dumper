#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file  sms.py
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
import struct

## ROM Operations
#
#  All Sega Master System specific functions

class sms:
    
    checksumRom = 0
    checksumCalc = 0
    
    headerAddress = 0x7FF0
    headerSize = 16
    headerData = {}
    
    readChunkSize = 2048
    progressBarSize = 64
    
    # relationship between rom size indicator in ROM header and header location
    # sizecode : (romSize, skipChecksumStart, skipChecksumEnd)
    romSizeData = {
        10 : (8192,    0x1FEF, 0x2000),
        11 : (16384,   0x3FEF, 0x4000),
        12 : (32768,   0x7FEF, 0x8000),
        13 : (49152,   0xBFEF, 0xC000),
        14 : (65536,   0x7FEF, 0x8000),
        15 : (131072,  0x7FEF, 0x8000),
        0  : (262144,  0x7FEF, 0x8000),
        1  : (525288,  0x7FEF, 0x8000),
        2  : (1048576, 0x7FEF, 0x8000), 
    }
    
    regionData = {
        3 : "SMS Japan",
        4 : "SMS Export",
        5 : "GG Japan",
        6 : "GG Export",
        7 : "GG International"
    }
    
########################################################################    
## The Constructor
#  \param self self
#
########################################################################
    def __init__(self):        
        pass
        
########################################################################    
## checksumSMS(self, file):
#  \param self self
#  \param file the rom to verify
#
########################################################################
    def checksum(self, filename):
        
        # SMS checksum skips the header portion (16 bytes at 0x7FF0), but 
        # starts calculating at 0
        pos = 0
        self.checksumCalc = 0
        fileSize = os.path.getsize(filename)

        with open(filename, "rb") as f:
            
            # read the ROM header's checksum value
            f.seek(0x7FFA, 0)
            data = f.read(2)
            thisWord = data[0], data[1]
            self.checksumRom = int.from_bytes(thisWord, byteorder="little")
            
            # read the ROM header's size info, some games put a smaller 
            # value here to speed up the checksum calculation
            f.seek(0x7FFF, 0)
            data = f.read(1)
            romSizeVal = int.from_bytes(data, byteorder="little") & 0x0F
            
            romSize = self.romSizeData.get(romSizeVal)[0]
            lowerBound = self.romSizeData.get(romSizeVal)[1]
            upperBound = self.romSizeData.get(romSizeVal)[2]
            
            # jump back to beginning of file
            f.seek(0, 0)
            
            while( pos < romSize ):
                if( ( romSize - pos ) >= self.readChunkSize):
                    sizeOfRead = self.readChunkSize
                else:
                    sizeOfRead = ( romSize - pos )
                    
                data = f.read(sizeOfRead)
                
                i = 0
                while i < sizeOfRead:
                    # check for lower and upper bound
                    if lowerBound < (pos + i) < upperBound:
                        # print("0x{0:X}".format(pos + i))
                        i += 1
                    else:
                        # int.from_bytes wasn't happy with an 8bit value
                        thisByte = data[i],0
                        intVal = int.from_bytes(thisByte, byteorder="little") & 0xFF
                        self.checksumCalc = (self.checksumCalc + intVal) & 0xFFFF
                        i += 1
                
                pos += sizeOfRead


########################################################################    
## decodeHeader
#  \param self self
#  
#  Read and format the ROM header for Sega Master System cartridge
########################################################################
    def formatHeader(self, filename):
        
        # clear current rom info dictionnary
        self.headerData.clear()
        with open(filename, "rb") as f:
            
        # get Trademark
            self.headerData.update({"Trademark": f.read(8).decode("utf-8", "replace") })
        # get checksum
            f.read(2) # 2 reserved bytes here, skip
            data = int.from_bytes(f.read(2), byteorder="little" )
            self.headerData.update({"Checksum": [data, hex(data)] })
        # get product code and version
            data = int.from_bytes(f.read(3), byteorder="little" ) & 0x0FFFFF
            self.headerData.update({"Product Code": [data, hex(data)] })
            
            version = (data >> 20) & 0x0F
            self.headerData.update({"Version": version })
        # get region and size
            data = int.from_bytes(f.read(1), byteorder="little" )
            regionVal = (data & 0xF0) >> 4
            print("{} {}".format(data, regionVal))
            self.headerData.update({"Region": self.regionData.get(regionVal) })

            romSizeVal = self.romSizeData.get((data & 0x0F))[0]
            self.headerData.update({"Size": [romSizeVal, hex(romSizeVal)] })
        
        return self.headerData
