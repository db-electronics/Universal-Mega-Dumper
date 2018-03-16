#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file  genesis.py
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

## Genesis
#
#  All Genesis specific functions
class genesis:

    checksumRom = 0
    checksumCalc = 0
    
    headerAddress = 0x100
    headerChecksum = 0x18E
    headerSize = 0x100
    headerData = {}
    romStartAddress = 0x200
    
    readChunkSize = 2048
    
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
## checksum(self, file):
#  \param self self
#  \param file the rom to verify
#
########################################################################
    def checksum(self, filename):
        
        # Genesis checksums start after the header
        pos = self.romStartAddress
        self.checksumCalc = 0
        fileSize = os.path.getsize(filename)
        
        with open(filename, "rb") as f:
            
            # read the ROM header's checksum value

            f.seek( self.headerChecksum, 0)
            data = f.read(2)
            thisWord = data[0],data[1]
            self.checksumRom = int.from_bytes(thisWord, byteorder="big")
            
            # jump ahead of ROM header
            f.seek(self.romStartAddress, 0)
            
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
## readGenesisROMHeader
#  \param self self
#  
#  Read and format the ROM header for Sega Genesis cartridge
########################################################################
    def formatHeader(self, filename):
        
        # clear current rom info dictionnary
        self.headerData.clear()
        with open(filename, "rb") as f:
            
        ## get console name
            self.headerData.update({"Console Name": f.read(16).decode("utf-8", "replace") })
        ## get copyright information
            self.headerData.update({"Copyright": f.read(16).decode("utf-8", "replace") })
        ## get domestic name
            self.headerData.update({"Domestic Name": f.read(48).decode("utf-8", "replace") })
        ## get overseas name
            self.headerData.update({"Overseas Name": f.read(48).decode("utf-8", "replace") })
        ## get serial number
            self.headerData.update({"Serial Number": f.read(14).decode("utf-8", "replace") })
        ## get checksum
            data = int.from_bytes(f.read(2), byteorder="big" )
            self.headerData.update({"Checksum": [data, hex(data)] })
        ## get io support
            self.headerData.update({"IO Support": f.read(16).decode("utf-8", "replace") })
        ## get ROM Start Address
            data = int.from_bytes(f.read(4), byteorder="big" )
            self.headerData.update({"ROM Begin": [data, hex(data)] })
        ## get ROM End Address
            data = int.from_bytes(f.read(4), byteorder="big" )
            self.headerData.update({"ROM End": [data, hex(data)] })
        ## get Start of RAM
            data = int.from_bytes(f.read(4), byteorder="big" )
            self.headerData.update({"RAM Begin": [data, hex(data)] })
        ## get End of RAM
            data = int.from_bytes(f.read(4), byteorder="big" )
            self.headerData.update({"RAM End": [data, hex(data)] })
        ## get sram support
            self.headerData.update({"SRAM Support": f.read(4) })
        ## get start of sram
            data = int.from_bytes(f.read(4), byteorder="big" )
            self.headerData.update({"SRAM Begin": [data, hex(data)] })
        ## get end of sram
            data = int.from_bytes(f.read(4), byteorder="big" )
            self.headerData.update({"SRAM End": [data, hex(data)] })
        ## get modem support
            self.headerData.update({"Modem Support": f.read(12).decode("utf-8", "replace") })
        ## get memo
            self.headerData.update({"Memo": f.read(40).decode("utf-8", "replace") })
        ## get country support
            self.headerData.update({"Country Support": f.read(16).decode("utf-8", "replace") })
        
        return self.headerData

