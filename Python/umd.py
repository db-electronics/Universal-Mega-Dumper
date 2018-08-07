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
import hashlib
import shutil
from xml.etree.ElementTree import iterparse
from hardware import umd
from genesis import genesis
from sms import sms
from snes import snes

# https://docs.python.org/3/howto/argparse.html

########################################################################    
## extractHeader(start, size, ifile, ofile):
#  \param self self
#  \param pos where to start reading the header
#  \param ifile the input ROM file
#  \param ofile the output header file
#
########################################################################
def extractHeader(start, size, ifile, ofile):
    try:
        os.remove(ofile)
    except OSError:
        pass
        
    with open(ofile, "wb+") as fwrite:
        with open(ifile, "rb") as fread:
            fread.seek(start, 0)
            readBytes = fread.read(size)
            fwrite.write(readBytes)

####################################################################################
## Main
####################################################################################
if __name__ == "__main__":
    # Some Windows codepages cause an exception in print
    # This re-opens stdout to use a replacing IO encoder
    # An alternative is to define env variable 
    #   PYTHONIOENCODING=:replace
    if sys.platform.startswith('win'):
        old_stdout = sys.stdout
        fd = os.dup(sys.stdout.fileno())
        sys.stdout = open(fd, mode='w', errors='replace')
        old_stdout.close()

    
    ## UMD Modes, names on the right must match values inside the classumd.py dicts
    carts = {"none" : "none",
            "cv" : "Colecovision",
            "gen" : "Genesis", 
            "sms" : "SMS",
            "pce" : "PCEngine",
            "tg" : "Turbografx-16",
            "snes" : "Super Nintendo" }
    
    parser = argparse.ArgumentParser(prog="umd 0.1.0.0")
    parser.add_argument("--mode", help="Set the cartridge type", choices=["cv", "gen", "sms", "pce", "tg", "snes"], type=str, default="none")
    parser.add_argument("--dat", nargs=1,help="The name of a 'ROM Management Datafile' XML file.  If a match is found in the dat, the output file will be renamed accordingly (or copied when --file is specified).", type=str)
    
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

    parser.add_argument("--port", 
                        help="Serial port name of UMD", 
                        type=str)
    
    args = parser.parse_args()
    
    # init UMD object, set console type
    cartType = carts.get(args.mode)
    #umd = umd(cartType, args.port)
    
    #if( args.mode != "none" ):
        ##print( "setting mode to {0}".format(carts.get(args.mode)) )
        #if( not(args.checksum & ( args.file != "console") ) ):
            #umd.connectUMD(cartType, args.port)
    
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
        
        # umd object declared explicitely in needed cases only
        
        # get the file list from the UMD's serial flash
        if args.rd == "sflist":
            umd = umd(cartType, args.port)
            umd.sfGetFileList()
            usedSpace = 0
            freeSpace = 0
            print("name           : bytes")
            print("--------------------------")
            for item in umd.sfFileList.items():
                #print(item)
                print("{0}: {1}".format(item[0].ljust(15), item[1]))
                usedSpace += item[1]
                
            freeSpace = umd.sfSize - usedSpace
            print("\n\r{0} of {1} bytes remaining".format(freeSpace, umd.sfSize))
        
        # read a file from the UMD's serial flash - the file is read in its entirety and output to a local file
        elif args.rd == "sf":
            if ( args.file == "console" ):
                print("must specify a local --FILE to write the serial flash file's contents into")
            else:
                umd = umd(cartType, args.port)
                umd.sfReadFile(args.sffile, args.file)
                print("read {0} from serial flash to {1} in {2:.3f} s".format(args.sfile, args.file, umd.opTime))
        
        # read the ROM header of the cartridge, output formatted in the console
        elif args.rd == "header":
            # Genesis Header
            if args.mode == "gen":
                genesis = genesis()
                # header from file or from UMD?
                if args.file != "console":
                    extractHeader(genesis.headerAddress, genesis.headerSize, args.file, "_header.bin")
                else:
                    umd = umd(cartType, args.port)
                    umd.read(genesis.headerAddress, genesis.headerSize, args.rd, "_header.bin")
                # display
                for item in sorted( genesis.formatHeader("_header.bin").items() ):
                    print(item)
                # cleanup
                try:
                    os.remove("_header.bin")
                except OSError:
                    pass
                del genesis
                
            # Master System Header        
            if args.mode == "sms":
                sms = sms()
                # header from file or from UMD?
                if args.file != "console":
                    extractHeader(sms.headerAddress, sms.headerSize, args.file, "_header.bin")
                else:
                    umd = umd(cartType, args.port)
                    umd.read(sms.headerAddress, sms.headerSize, args.rd, "_header.bin")
                    
                for item in sorted( sms.formatHeader("_header.bin").items() ):
                    print(item)
                # cleanup
                try:
                    os.remove("_header.bin")
                except OSError:
                    pass
                del sms
            
            # Super Nintendo Header
            if args.mode == "snes":
                snes = snes()                
                # header could be in different locations, iterate until we find it
                # still testing this part - not functional!
                for key, value in snes.header.items():
                    print("key: {}  value : {}".format(key, value))
                    
                del snes
                
        # read the flash id - obviously does not work on original games with OTP roms
        elif args.rd == "fid":
            umd = umd(cartType, args.port)
            umd.getFlashID()
            for item in sorted( umd.flashIDData.items() ):
                print(item)
            
        # read the serial flash id
        elif args.rd == "sfid":
            umd = umd(cartType, args.port)
            umd.sfGetId()
            print (''.join('0x{:02X} '.format(x) for x in umd.sfID))
                
        # read from the cartridge, ROM/SAVE/BRAM specified in args.rd
        else:
            umd = umd(cartType, args.port)
            print("reading {0} bytes starting at address 0x{1:X} from {2} {3}".format(byteCount, address, cartType, args.rd))
            ofile = args.file
            if args.file == "console" and args.dat:
                ofile = "_temp.bin"
            umd.read(address, byteCount, args.rd, ofile)
            print("read {0} bytes completed in {1:.3f} s".format(byteCount, umd.opTime))
            if args.dat:
                matched_name = ""
                with open(ofile, 'rb') as in_file:
                    romdata = in_file.read()
                    sha1sum = hashlib.sha1(romdata).hexdigest().upper()
                    for _, elem in iterparse(args.dat[0]):
                        if elem.tag == 'rom':
                            if elem.attrib['sha1'] and elem.attrib['sha1'] == sha1sum:
                                print("Found match in dat: " + elem.attrib['name'])
                                matched_name = elem.attrib['name']
                                break
                        elem.clear()
                if matched_name is not "":
                    try:
                        os.remove(matched_name)
                    except OSError:
                        pass
                    if args.file == "console":
                        os.rename(ofile, matched_name)
                    else:
                        shutil.copyfile(ofile, matched_name)
                else:
                    print("Unable to find a match in {0}.  File left in {1}".format(args.dat[0],ofile))


    # clear operations - erase various memories
    elif args.clr:
        # currently always need the hardware for clear
        umd = umd(cartType, args.port)
        
        # erase the entire serial flash
        if args.clr == "sf":
            print("erasing serial flash...")
            umd.sfEraseAll()
            print("erase serial flash completed in {0:.3f} s".format(umd.opTime))
        
        # erase the save memory on a cartridge, create an empty file filled with zeros and write it to the save
        elif args.clr == "save":
            print("erasing save memory...")
            with open("zeros.bin", "wb+") as f:
                f.write(bytes(byteCount))
            umd.write(address, "save", "zeros.bin")
            try:
                os.remove("zeros.bin")
            except OSError:
                pass
            print("cleared {0} bytes of save at address 0x{1:X} in {2:.3f} s".format(byteCount, address, umd.opTime))
            
        # erase the flash rom on a cartridge, some cart types have multiple chips, need to figure out if more than 1 is connected
        elif args.clr == "rom":
            print("erasing flash chip...")
            umd.eraseChip(0)
            print("erase flash chip completed in {0:.3f} s".format(umd.opTime))
                
            
    # write operations
    elif args.wr:
        # currently always need the hardware for writes
        umd = umd(cartType, args.port)
        
        # write a local file to the UMD's serial flash
        if args.wr == "sf":
            if args.file == "console":
                print("must specify a --FILE to write to the serial flash")
            else:
                print("writing {0} to serial flash as {1}...".format(args.file, args.sfile))
                umd.sfWriteFile(args.sfile, args.file)
                print("wrote {0} to serial flash as {1} in {2:.3f} s".format(args.file, args.sfile, umd.opTime))
    
        # write to the cartridge's flash rom
        elif args.wr == "rom":
            
            # first check if a local file is to be written to the cartridge
            if args.file != "console":
                print("burning {0} contents to ROM at address 0x{1:X}".format(args.file, address))
                umd.write(address, args.wr, args.file)
                print("burn {0} completed in {1:.3f} s".format(args.file, umd.opTime))
                
            else:
                if args.sfile:
                    umd.sfBurnCart(args.sfile)
                    print("burned {0} from serial flash to {1} in {2:.3f} s".format(args.sfile, cartType, umd.opTime))

        # all other writes handled here
        else:
            umd.write(address, args.wr, args.file)
            print("wrote to {0} in {1:.3f} s".format(args.wr, umd.opTime))

    # checksum operations
    elif args.checksum:
        
        # Genesis Checksum
        if args.mode == "gen":
            startTime = time.time()
            genesis = genesis()
            
            # checksum of local file or of cartridge on UMD?
            if args.file != "console":
                genesis.checksum(args.file)
                
            else:
                #  read rom size from header
                umd = umd(cartType, args.port)
                umd.read(genesis.headerAddress, genesis.headerSize, args.rd, "_header.bin")
                romSize = genesis.formatHeader("_header.bin").get("ROM End")[0] + 1
                print("Found ROM size = {} bytes in cartridge header, reading in...".format(romSize))
                #  read rom into local file
                umd.read(0, romSize, args.rd, "_temp.bin")
                #  pass that file to checksum
                genesis.checksum("_temp.bin")
                #  delete local files
                try:
                    os.remove("_header.bin")
                    os.remove("_temp.bin")
                except OSError:
                    pass

            opTime = time.time() - startTime
            print("checksum completed in {0:.3f} s, calculated 0x{1:X}, value in header is 0x{2:X}".format(opTime, genesis.checksumCalc, genesis.checksumRom)) 
            # cleanup
            del genesis    
            
        # Master System Checksum
        if args.mode == "sms":
            startTime = time.time()
            sms = sms()
            
            # checksum of local file or of cartridge on UMD?
            if args.file != "console":
                sms.checksum(args.file)
                
            else:
                #  read rom size from header
                umd = umd(cartType, args.port)
                umd.read(sms.headerAddress, sms.headerSize, args.rd, "_header.bin")
                romSize = sms.formatHeader("_header.bin").get("Size")[0]
                print("Found ROM size = {} bytes in cartridge header, reading in...".format(romSize))
                #  read rom into local file
                umd.read(0, romSize, args.rd, "_temp.bin")
                #  pass that file to checksum
                sms.checksum("_temp.bin")
                #  delete local files
                try:
                    os.remove("_header.bin")
                    os.remove("_temp.bin")
                except OSError:
                    pass
            
            opTime = time.time() - startTime
            print("checksum completed in {0:.3f} s, calculated 0x{1:X}, value in header is 0x{2:X}".format(opTime, sms.checksumCalc, sms.checksumRom)) 
            # cleanup
            del sms


    elif args.byteswap:
        startTime = time.time()
        genesis = genesis()
        genesis.byteSwap(args.byteswap[0], args.file)
        opTime = time.time() - startTime
        del genesis
    else:
        parser.print_help()
        pass

