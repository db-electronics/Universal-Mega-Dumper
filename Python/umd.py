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
            "tg16" : "Turbografx-16" }
    
    parser = argparse.ArgumentParser(prog="umd 0.1.0.0")
    parser.add_argument("--mode", help="Set the cartridge type", choices=["cv", "gen", "sms", "pce", "tg16"], type=str, default="none")
    
    readWriteArgs = parser.add_mutually_exclusive_group()
    readWriteArgs.add_argument("--getid", help="Get Flash ID from cartridge", action="store_true")
    readWriteArgs.add_argument("--sfid", help="Get Serial Flash ID from dumper", action="store_true")
    readWriteArgs.add_argument("--sferase", help="Erase Serial Flash", action="store_true")
    readWriteArgs.add_argument("--sflist", help="List Serial Flash Files", action="store_true")
    readWriteArgs.add_argument("--sfwrite", nargs=1, help="Write FILE to Serial Flash", type=str, metavar=('filename'))
    readWriteArgs.add_argument("--sfburn", nargs=1, help="Burn Serial Flash filename to cartridge", type=str, metavar=('filename'))
    readWriteArgs.add_argument("--checksum", help="Calculate ROM checksum", action="store_true")
    
    readWriteArgs.add_argument("--read", nargs=2, help="Read BYTECOUNT from ADDRESS, to FILE or console if not specified", type=str, metavar=('bytecount', 'address'))
    readWriteArgs.add_argument("--write", nargs=2, help="Write VALUE at ADDRESS", type=str, metavar=('value', 'address'))
    readWriteArgs.add_argument("--savewrite", nargs=1, help="Write FILE to SAVE MEMORY", type=str, metavar=('address'))
    readWriteArgs.add_argument("--cdwrite", nargs=1, help="Write FILE to CD MEMORY", type=str, metavar=('address'))
    readWriteArgs.add_argument("--program", nargs=2, help="Program VALUE at ADDRESS", type=str, metavar=('value', 'address'))
    readWriteArgs.add_argument("--burn", nargs=1, help="Burn FILE contents at ADDRESS", type=str, metavar=('address'))
    readWriteArgs.add_argument("--erase", nargs=1, help="Erase entire ROM contents", type=str, metavar=('chip'))
    
    readWriteArgs.add_argument("--swapbytes", nargs=1, help="Reverse the endianness of a file", type=str, metavar=('new file'))
    
    readWriteArgs.add_argument("--rd", 
                                help="Read from UMD", 
                                choices=["rom", "save", "bram", "sffile", "sflist", "header"], 
                                type=str)
    
    readWriteArgs.add_argument("--wr", 
                                help="Write to UMD", 
                                choices=["rom", "save", "bram", "sffile", "header"], 
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
    
    parser.add_argument("--file", help="File path for read/write operations", type=str, default="console")
    parser.add_argument("--sffile", help="8.3 filename for UMD's serial flash", type=str)
    parser.add_argument("--ram", help="Target RAM/ROM memory on cartridge", action="store_true")
    
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
    
    #read operations
    if args.rd:
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
        
        elif args.rd == "sffile":
            if ( args.file == "console" ):
                print("must specify a local --FILE to write the serial flash file's contents into")
            else:
                dumper.sfReadFile(args.sffile, args.file)
                print("read {0} from serial flash to {1} in {2:.3f} s".format(args.sffile, args.file, dumper.opTime))
            
        elif args.rd == "header":
            if args.mode == "gen":
                dumper.readGenesisROMHeader()
            if args.mode == "sms":
                dumper.readSMSROMHeader()
            
            for item in sorted(dumper.romInfo.items()):
                print(item)    
        else:
            address = int(args.addr[0], 0)
            print("reading {0} bytes starting at address 0x{1:X} from {2}".format(byteCount, address, console))
            dumper.readnew(address, byteCount, dataWidth, args.rd, args.file)
            print("read {0} bytes completed in {1:.3f} s".format(byteCount, dumper.opTime))
    
    elif args.getid:
        dumper.getFlashID(dataWidth)
        print("0x{0:0{1}X}".format(dumper.flashID,8))
        #print(dumper.flashID)

    elif args.sfid:
        dumper.sfGetId()
        print (''.join('0x{:02X} '.format(x) for x in dumper.sfID))
        #print(dumper.sfID)
    
    elif args.sferase:
        print("erasing serial flash...")
        dumper.sfEraseAll()
        print("erase serial flash completed in {0:.3f} s".format(dumper.opTime))
    
    elif args.sfwrite:
        sfFilename = args.sfwrite[0]
        filename = args.file
        if ( filename == "console" ):
            print("must specify a --FILE to write to the serial flash")
        else:
            print("writing {0} to serial flash...".format(filename))
            dumper.sfWriteFile(sfFilename, filename)
            print("wrote {0} to serial flash as {1} in {2:.3f} s".format(filename, sfFilename, dumper.opTime))
    
    elif args.sfburn:
        sfFilename = args.sfburn[0]
        dumper.sfBurnCart(sfFilename)
        print("burned {0} from serial flash to {1} in {2:.3f} s".format(sfFilename, console, dumper.opTime))
    
    elif args.sfread:
        sfFilename = args.sfread[0]
        filename = args.file
        if ( filename == "console" ):
            print("must specify a --FILE to write to the serial flash")
        else:
            dumper.sfReadFile(sfFilename, filename)
            print("read {0} from serial flash to {1} in {2:.3f} s".format(sfFilename, filename, dumper.opTime))
    
    elif args.erase:
        eraseChips = args.erase[0].split(",")
        for eraseChip in eraseChips:    
            print("erasing flash chip {0}...".format(eraseChip))
            dumper.eraseChip(eraseChip)
            print("erase flash chip {0} completed in {1:.3f} s".format(eraseChip, dumper.opTime))
    
    elif args.checksum:
        if mode == "gen":
            dumper.checksumGenesis()
            print("checksum completed in {0:.3f} s, calculated {1} expected {2}".format(dumper.opTime, dumper.checksumCalculated, dumper.checksumCart)) 
        if mode == "sms":
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
        filename = args.file
        address = int(args.savewrite[0], 0)
        dumper.saveWrite(filename, address, dataWidth)
        print("wrote {0} to SRAM in {1:.3f} s".format(filename, dumper.opTime))
    
    elif args.cdwrite:
        filename = args.file
        address = int(args.cdwrite[0], 0)
        dumper.cdWrite(filename, address)
        print("wrote {0} to CD RAM in {1:.3f} s".format(filename, dumper.opTime))
        
    elif args.burn:
        filename = args.file
        address = int(args.burn[0], 0)
        print("burning {0} contents to ROM at address 0x{1:X}".format(filename, address))
        dumper.burn(filename, address, dataWidth)
        print("burn {0} completed in {1:.3f} s".format(filename, dumper.opTime))

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

