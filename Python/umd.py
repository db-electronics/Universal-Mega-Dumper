#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file umd.py
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
from classumd import umd
from romopsumd import romOperations

# https://docs.python.org/3/howto/argparse.html

####################################################################################
## Main
####################################################################################
if __name__ == "__main__":
    
    ## UMD Modes, names on the right must match values inside the classumd.py dicts
    carts = {"none" : "none",
            "cv" : "Colecovision",
            "gen" : "Genesis", 
            "sms" : "SMS",
            "pce" : "PCEngine",
            "tg" : "Turbografx-16" }
    
    parser = argparse.ArgumentParser(prog="umd 0.1.0.0")
    parser.add_argument("--mode", help="Set the cartridge type", choices=["cv", "gen", "sms", "pce", "tg"], type=str, default="none")
    
    readWriteArgs = parser.add_mutually_exclusive_group()
    readWriteArgs.add_argument("--checksum", help="Calculate ROM checksum", action="store_true")

    readWriteArgs.add_argument("--byteswap", nargs=1, help="Reverse the endianness of a file", type=str, metavar=('file to byte swap'))
    
    readWriteArgs.add_argument("--rd", 
                                help="Read from UMD", 
                                choices=["rom", "save", "bram", "header", "fid", "sfid", "sf", "sflist", "byte", "word", "sbyte", "sword"], 
                                type=str)
    
    readWriteArgs.add_argument("--wr", 
                                help="Write to UMD", 
                                choices=["rom", "save", "bram", "sf"], 
                                type=str)
    
    readWriteArgs.add_argument("--clr", 
                                help="Clear a memory in the UMD", 
                                choices=["rom", "rom2", "save", "bram", "sf"], 
                                type=str)
    
    parser.add_argument("--addr", 
                        nargs=1, 
                        help="Address for current command", 
                        type=str,
                        default="0")
    
    parser.add_argument("--size", 
                        nargs=1, 
                        help="Size in bytes for current command", 
                        type=str,
                        default="1")
    
    parser.add_argument("--file", 
                        help="File path for read/write operations", 
                        type=str, 
                        default="console")
                        
    parser.add_argument("--sfile", 
                        help="8.3 filename for UMD's serial flash", 
                        type=str)
    
    args = parser.parse_args()
    
    # init UMD object, set console type
    cartType = carts.get(args.mode)
    dumper = umd(cartType)
    
    if( args.mode != "none" ):
        #print( "setting mode to {0}".format(carts.get(args.mode)) )
        if( not(args.checksum & ( args.file != "console") ) ):
            dumper.connectUMD(cartType)
    
    #figure out the size of the operation, default to 1 in arguments so OK to calc everytime
    if( args.size[0].endswith("Kb") ):  
        byteCount = int(args.size[0][:-2], 0) * (2**7)
    elif( args.size[0].endswith("KB") ):
        byteCount = int(args.size[0][:-2], 0) * (2**10)
    elif( args.size[0].endswith("Mb") ):
        byteCount = int(args.size[0][:-2], 0) * (2**17)
    elif( args.size[0].endswith("MB") ):
        byteCount = int(args.size[0][:-2], 0) * (2**20)
    else:
        byteCount = int(args.size[0], 0)
    
    address = int(args.addr[0], 0)
    
    # read operations
    if args.rd:
        
        # get the file list from the UMD's serial flash
        if args.rd == "sflist":
            dumper.sfGetFileList()
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
        
        # read a file from the UMD's serial flash - the file is read in its entirety and output to a local file
        elif args.rd == "sf":
            if ( args.file == "console" ):
                print("must specify a local --FILE to write the serial flash file's contents into")
            else:
                dumper.sfReadFile(args.sffile, args.file)
                print("read {0} from serial flash to {1} in {2:.3f} s".format(args.sfile, args.file, dumper.opTime))
        
        # read the ROM header of the cartridge, output formatted in the console
        elif args.rd == "header":
            if args.mode == "gen":
                dumper.readGenesisROMHeader()
            if args.mode == "sms":
                dumper.readSMSROMHeader()
            
            for item in sorted(dumper.romInfo.items()):
                print(item)
        
        # read the flash id - obviously does not work on original games with OTP roms
        elif args.rd == "fid":
            dumper.getFlashID()
            print("0x{0:0{1}X}".format(dumper.flashID,8))
            
        # read the serial flash id
        elif args.rd == "sfid":
            dumper.sfGetId()
            print (''.join('0x{:02X} '.format(x) for x in dumper.sfID))
                
        # read from the cartridge, ROM/SAVE/BRAM specified in args.rd
        else:
            print("reading {0} bytes starting at address 0x{1:X} from {2} {3}".format(byteCount, address, cartType, args.rd))
            dumper.read(address, byteCount, args.rd, args.file)
            print("read {0} bytes completed in {1:.3f} s".format(byteCount, dumper.opTime))

    # clear operations - erase various memories
    elif args.clr:
        
        # erase the entire serial flash
        if args.clr == "sf":
            print("erasing serial flash...")
            dumper.sfEraseAll()
            print("erase serial flash completed in {0:.3f} s".format(dumper.opTime))
        
        # erase the save memory on a cartridge, create an empty file filled with zeros and write it to the save
        elif args.clr == "save":
            print("erasing save memory...")
            with open("zeros.bin", "wb+") as f:
                f.write(bytes(byteCount))
            dumper.write(address, "save", "zeros.bin")
            try:
                os.remove("zeros.bin")
            except OSError:
                pass
            print("cleared {0} bytes of save at address 0x{1:X} in {2:.3f} s".format(byteCount, address, dumper.opTime))
            
        # erase the flash rom on a cartridge, some cart types have multiple chips, need to figure out if more than 1 is connected
        elif args.clr == "rom":
            if args.mode == "gen":
                for eraseChip in range(0,2):    
                    print("erasing flash chip {0}...".format(eraseChip))
                    dumper.eraseChip(eraseChip)
                    print("erase flash chip {0} completed in {1:.3f} s".format(eraseChip, dumper.opTime))
            else:
                print("erasing flash chip...")
                dumper.eraseChip(0)
                print("erase flash chip completed in {0:.3f} s".format(dumper.opTime))
            
    # write operations
    elif args.wr:
    
        # write a local file to the UMD's serial flash
        if args.wr == "sf":
            if args.file == "console":
                print("must specify a --FILE to write to the serial flash")
            else:
                print("writing {0} to serial flash as {1}...".format(args.file, args.sfile))
                dumper.sfWriteFile(args.sfile, args.file)
                print("wrote {0} to serial flash as {1} in {2:.3f} s".format(args.file, args.sfile, dumper.opTime))
    
        # write to the cartridge's flash rom
        elif args.wr == "rom":
            # first check if a local file is to be written to the cartridge
            if args.file != "console":
                print("burning {0} contents to ROM at address 0x{1:X}".format(args.file, address))
                dumper.write(address, args.wr, args.file)
                print("burn {0} completed in {1:.3f} s".format(args.file, dumper.opTime))
            else:
                if args.sfile:
                    dumper.sfBurnCart(args.sfile)
                    print("burned {0} from serial flash to {1} in {2:.3f} s".format(args.sfile, cartType, dumper.opTime))

        # all other writes handled here
        else:
            dumper.write(address, args.wr, args.file)
            print("wrote to {0} in {1:.3f} s".format(args.wr, dumper.opTime))

    # checksum operations
    elif args.checksum:
        if args.mode == "gen":
            if args.file:
                startTime = time.time()
                romOps = romOperations()
                checksum = romOps.checksumGenesis(args.file)
                opTime = time.time() - startTime
                print("checksum completed in {0:.3f} s, calculated 0x{1:X} expected 0x{2:X}".format(opTime, romOps.checksumCalc, romOps.checksumRom)) 
                del romOps
            else:
                pass
                
        if args.mode == "sms":
            if args.file:
                startTime = time.time()
                romOps = romOperations()
                checksum = romOps.checksumSMS(args.file)
                opTime = time.time() - startTime
                print("checksum completed in {0:.3f} s, calculated 0x{1:X} expected 0x{2:X}".format(opTime, romOps.checksumCalc, romOps.checksumRom)) 
                del romOps
            else:
                pass


    elif args.byteswap:
        startTime = time.time()
        romOps = romOperations()
        romOps.byteSwap(args.byteswap[0], args.file)
        opTime = time.time() - startTime
        del romOps
    else:
        parser.print_help()
        pass

