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
    parser.add_argument("--mode", help="Set the cartridge type", choices=["cv", "gen", "sms", "pce", "tg16"], type=str, default="none")
    
    readWriteArgs = parser.add_mutually_exclusive_group()
    readWriteArgs.add_argument("--sfburn", nargs=1, help="Burn Serial Flash filename to cartridge", type=str, metavar=('filename'))
    readWriteArgs.add_argument("--checksum", help="Calculate ROM checksum", action="store_true")

    readWriteArgs.add_argument("--savewrite", nargs=1, help="Write FILE to SAVE MEMORY", type=str, metavar=('address'))
    readWriteArgs.add_argument("--cdwrite", nargs=1, help="Write FILE to CD MEMORY", type=str, metavar=('address'))
    readWriteArgs.add_argument("--burn", nargs=1, help="Burn FILE contents at ADDRESS", type=str, metavar=('address'))
    
    readWriteArgs.add_argument("--swapbytes", nargs=1, help="Reverse the endianness of a file", type=str, metavar=('new file'))
    
    readWriteArgs.add_argument("--rd", 
                                help="Read from UMD", 
                                choices=["rom", "save", "bram", "header", "fid", "sfid", "sf", "sflist"], 
                                type=str)
    
    readWriteArgs.add_argument("--wr", 
                                help="Write to UMD", 
                                choices=["rom", "save", "bram", "sf", "header"], 
                                type=str)
    
    readWriteArgs.add_argument("--clr", 
                                help="Clear a memory in the UMD", 
                                choices=["rom", "save", "bram", "sf"], 
                                type=str)
    
    parser.add_argument("--addr", 
                        nargs=1, 
                        help="Address for current command", 
                        type=str, 
                        metavar=('address'),
                        default="0")
    
    parser.add_argument("--size", 
                        nargs=1, 
                        help="Size in bytes for current command", 
                        type=str, 
                        metavar=('size'),
                        default="1")
    
    parser.add_argument("--file", 
                        help="File path for read/write operations", 
                        type=str, 
                        default="console")
                        
    parser.add_argument("--sfile", 
                        help="8.3 filename for UMD's serial flash", 
                        type=str)
    
    args = parser.parse_args()
    
    dumper = umd()
    #print( dumper.modes )
    #print( dumper.modes.get("Genesis") )
    
    if( args.mode != "none" ):
        #print( "setting mode to {0}".format(carts.get(args.mode)) )
        dumper.connectUMD( carts.get(args.mode) )
    
    dataWidth = dumper.busWidth.get( carts.get(args.mode) )
    console = carts.get(args.mode)
    
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
            dumper.getFlashID(dataWidth)
            print("0x{0:0{1}X}".format(dumper.flashID,8))
            
        elif args.rd == "sfid":
            dumper.sfGetId()
            print (''.join('0x{:02X} '.format(x) for x in dumper.sfID))
                
        # read from the cartridge, ROM/SAVE/BRAM specified in args.rd
        else:
            print("reading {0} bytes starting at address 0x{1:X} from {2}".format(byteCount, address, console))
            dumper.readnew(address, byteCount, dataWidth, args.rd, args.file)
            print("read {0} bytes completed in {1:.3f} s".format(byteCount, dumper.opTime))

    # clear operations - erase various memories
    elif args.clr:
        
        # erase the entire serial flash
        if args.clr == "sf":
            print("erasing serial flash...")
            dumper.sfEraseAll()
            print("erase serial flash completed in {0:.3f} s".format(dumper.opTime))
            
        # erase the flash rom on a cartridge, some cart types have multiple chips, need to figure out if more than 1 is connected
        elif args.clr == "rom":
            if args.mode == "gen":
                for eraseChip in range(0,1):    
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
                dumper.burn(args.file, address, dataWidth)
                print("burn {0} completed in {1:.3f} s".format(args.file, dumper.opTime))
            else:
                if args.sfile:
                    dumper.sfBurnCart(args.sfile)
                    print("burned {0} from serial flash to {1} in {2:.3f} s".format(args.sfile, console, dumper.opTime))
                    

    elif args.burn:
        filename = args.file
        address = int(args.burn[0], 0)
        print("burning {0} contents to ROM at address 0x{1:X}".format(filename, address))
        dumper.burn(filename, address, dataWidth)
        print("burn {0} completed in {1:.3f} s".format(filename, dumper.opTime))
    
    elif args.sfburn:
        sfFilename = args.sfburn[0]
        dumper.sfBurnCart(sfFilename)
        print("burned {0} from serial flash to {1} in {2:.3f} s".format(sfFilename, console, dumper.opTime))

    elif args.checksum:
        if mode == "gen":
            dumper.checksumGenesis()
            print("checksum completed in {0:.3f} s, calculated {1} expected {2}".format(dumper.opTime, dumper.checksumCalculated, dumper.checksumCart)) 
        if mode == "sms":
            dumper.checksumSMS()
            print("checksum completed in {0:.3f} s, calculated {1} expected {2}".format(dumper.opTime, dumper.checksumCalculated, dumper.checksumCart)) 
    
    elif args.savewrite:
        filename = args.file
        address = int(args.savewrite[0], 0)
        dumper.saveWrite(filename, address, dataWidth)
        print("wrote {0} to SRAM in {1:.3f} s".format(filename, dumper.opTime))
    
    elif args.cdwrite:
        filename = args.file
        address = int(args.cdwrite[0], 0)
        dumper.cdWrite(filename, address)
        print("wrote {0} to CD RAM in {1:.3f} s".format(filename, dumper.opTime))

    elif args.swapbytes:
        filename = args.file
        outFile = args.swapbytes[0]
        fileSize = os.path.getsize(filename)
        pos = 0
        
        try:
            os.remove(outFile)
        except OSError:
            pass
            
        with open(outFile, "wb+") as fwrite:
            with open(filename, "rb") as fread:
                while(pos < fileSize):  
                    badEndian = fread.read(2)
                    revEndian = struct.pack('<h', *struct.unpack('>h', badEndian))
                    fwrite.write(revEndian)
                    pos += 2
        

    else:
        parser.print_help()
        pass

