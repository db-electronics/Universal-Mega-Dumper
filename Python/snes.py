#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file  snes.py
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

## SNES
#
#  All Super Nintendo specific functions
class snes:

    header = {"LoROM" : 0x7FC0,
            "HiROM" : 0xFFC0 }
            
    headerSize = 64

    checksumRom = 0
    checksumCalc = 0
    romInfo = {}
    
########################################################################    
## The Constructor
#  \param self self
#
########################################################################
    def __init__(self):        
        pass


########################################################################    
## getLoROMAddress(self, address):
#  \param self self
#  \param address the real ROM address
#
#  Calculate the virtual ROM for LoROM carts - A15 is NC therefore
#  every other 32KB is a mirror
########################################################################
    def getLoROMAddress(self, address):

        return ((address << 1) & 0xFFFF0000) | (address & 0x00007FFF)


########################################################################    
## readHeader
#  \param self self
#  
#  Read and format the ROM header for Super Nintendo cartridge
########################################################################
    def readHeader(self):
        
        # clear current rom info dictionnary
        self.romInfo.clear()

        # header data could be in one of two places, 0x7FC0 or 0xFFC0
        # search for 21 ASCII characters at the beginning of the header
        
        # check for valid header at 0x7FC0
        cmd = "rdbblk 0x7FC0 21\r\n"
        self.serialPort.write(bytes(cmd,"utf-8"))
        response = self.serialPort.read(21)

        headerLo = True
        for testChar in response:
            if not(0x20 <= testChar <= 0x7F):
                print("invalid ascii char {0} found in 0x7FC0 header".format(testChar))
                headerLo = False
                break
        
        if headerLo:
            response = response.decode("utf-8", "replace")
            print("{0} is plausibly the game's title found in 0x7FC0 header".format(response))

        else:
            # check for valid header at 0x7FC0
            cmd = "rdbblk 0x7FF0 21\r\n"
            self.serialPort.write(bytes(cmd,"utf-8"))
            response = self.serialPort.read(21)

            headerHi = True
            for testChar in response:
                if not(0x20 <= testChar <= 0x7F):
                    print("invalid ascii char {0} found in 0xFFC0 header".format(testChar))
                    headerLo = False
                    break




