#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file  romopsumd.py
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
#  All communications with the UMD are handled by the umd class, as well
#  as several ROM header read and checksum calculation routines.
class romOperations:
    
    checksumRom = 0
    checksumCalc = 0
    
    readChunkSize = 2048
    progressBarSize = 64
    
########################################################################    
## The Constructor
#  \param self self
#
########################################################################
    def __init__(self):        
        pass

########################################################################    
## byteSwap(self, ifile, ofile):
#  \param self self
#  \param ifile
#  \param ofile
#
########################################################################
    def byteSwap(self, ifile, ofile):
        
        fileSize = os.path.getsize(ifile)
        pos = 0
        
        try:
            os.remove(ofile)
        except OSError:
            pass
            
        with open(ofile, "wb+") as fwrite:
            with open(ifile, "rb") as fread:
                while(pos < fileSize):  
                    badEndian = fread.read(2)
                    revEndian = struct.pack('<h', *struct.unpack('>h', badEndian))
                    fwrite.write(revEndian)
                    pos += 2

########################################################################    
## checksumGenesis(self, file):
#  \param self self
#  \param file the rom to verify
#
########################################################################
    def checksumGenesis(self, filename):
        
        # Genesis checksums start after the header
        pos = 512
        self.checksumCalc = 0
        fileSize = os.path.getsize(filename)
        
        with open(filename, "rb") as f:
            
            # read the ROM header's checksum value

            f.seek( 0x018E, 0)
            data = f.read(2)
            thisWord = data[0],data[1]
            self.checksumRom = int.from_bytes(thisWord, byteorder="big")
            
            # jump ahead of ROM header
            f.seek(pos, 0)
            
            while( pos < fileSize ):
                if( ( fileSize - pos ) >= self.readChunkSize):
                    sizeOfRead = self.readChunkSize
                else:
                    sizeOfRead = ( fileSize - pos )
                    
                data = f.read(sizeOfRead)
                
                i = 0
                while i < sizeOfRead:
                    thisWord = data[i],data[i + 1]
                    intVal = int.from_bytes(thisWord, byteorder="big")
                    self.checksumCalc = (self.checksumCalc + intVal) & 0xFFFF
                    i += 2 
                
                pos += sizeOfRead

########################################################################    
## checksumSMS(self, file):
#  \param self self
#  \param file the rom to verify
#
########################################################################
    def checksumSMS(self, filename):
        
        # SMS checksum skips the header portion (16 bytes at 0x7FF0), but 
        # starts calculating at 0
        pos = 0
        self.checksumCalc = 0
        fileSize = os.path.getsize(filename)

        with open(filename, "rb") as f:
            
            # read the ROM header's checksum value
            f.seek( 0x7FFA, 0)
            data = f.read(2)
            thisWord = data[0],data[1]
            self.checksumRom = int.from_bytes(thisWord, byteorder="little")
            
            # read the ROM header's size info, some games put a smaller 
            # value here to speed up the checksum calculation
            f.seek( 0x7FFF, 0)
            data = f.read(1)
            romSizeVal = int.from_bytes(data, byteorder="little") & 0x0F
            
            if romSizeVal == 10:
                romsize = 8192
                lowerBound = 0x1FEF
                upperBound = 0x2000
            elif romSizeVal == 11:
                romsize = 16384
                lowerBound = 0x3FEF
                upperBound = 0x4000
            elif romSizeVal == 12:
                romsize = 32768
                lowerBound = 0x7FEF
                upperBound = 0x8000
            elif romSizeVal == 13:
                romsize = 49152
                lowerBound = 0xBFEF
                upperBound = 0xC000
            elif romSizeVal == 14:
                romsize = 65536
                lowerBound = 0x7FEF
                upperBound = 0x8000
            elif romSizeVal == 15:
                romsize = 131072
            elif romSizeVal == 0:
                lowerBound = 0x7FEF
                upperBound = 0x8000
                romsize = 262144
            elif romSizeVal == 1:
                romsize = 525288
                lowerBound = 0x7FEF
                upperBound = 0x8000
            elif romSizeVal == 2:
                romsize = 1048576
                lowerBound = 0x7FEF
                upperBound = 0x8000
            else:
                romsize = 0
            
            # print("filesize = {0}".format(fileSize) )
            # print("romsize = {0}".format(romsize) )
            
            # jump back to beginning of file
            f.seek(0,
            
            while( pos < romsize ):
                if( ( romsize - pos ) >= self.readChunkSize):
                    sizeOfRead = self.readChunkSize
                else:
                    sizeOfRead = ( romsize - pos )
                    
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



