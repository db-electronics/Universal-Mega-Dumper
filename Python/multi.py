#! /usr/bin/env python
# -*- coding: utf-8 -*-
########################################################################
# \file multi.py
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

from subprocess import Popen, PIPE

# https://docs.python.org/3/howto/argparse.html

####################################################################################
## Main
####################################################################################
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(prog="multi 0.1.0.0")
    parser.add_argument("--mode", help="Set the cartridge type", choices=["cv", "gen", "sms", "pce", "tg", "snes"], type=str, default="none")
    
    readWriteArgs = parser.add_mutually_exclusive_group()
    readWriteArgs.add_argument("--checksum", help="Calculate ROM checksum", action="store_true")
   
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
    
    multiArgs = [sys.executable, 'umd.py', '--mode', args.mode]
        
    # read operations
    if args.rd:
    	multiArgs.append("--rd")
    	multiArgs.append(args.rd)
    # clear operations - erase various memories
    elif args.clr:
    	multiArgs.append("--clr")
    	multiArgs.append(args.clr)
    # write operations
    elif args.wr:
    	multiArgs.append("--wr")
    	multiArgs.append(args.wr)
    # checksum operations
    elif args.checksum:
    	multiArgs.append("--checksum")
    else:
        parser.print_help()
        sys.exit()
    
    if args.addr is not None:
        multiArgs.append("--addr")
        if isinstance(args.addr, list):
            multiArgs.append(args.addr[0])
        else:
            multiArgs.append(args.addr)
    
    if args.size is not None:
        multiArgs.append("--size")
        if isinstance(args.size, list):
            multiArgs.append(args.size[0])
        else:
            multiArgs.append(args.size)
    
    if args.file is not None:
        multiArgs.append("--file")
        multiArgs.append(args.file)
    
    if args.sfile is not None:
        multiArgs.append("--sfile")
        multiArgs.append(args.sfile)

	# the port force argument
    multiArgs.append("--port")

    # enumerate ports
    if sys.platform.startswith("win"):
        ports = ["COM%s" % (i + 1) for i in range(256)]
    elif sys.platform.startswith ("linux"):
        ports = glob.glob("/dev/tty[A-Za-z]*")
    elif sys.platform.startswith ("darwin"):
        ports = glob.glob("/dev/cu*")
    else:
        raise EnvironmentError("Unsupported platform")

    umdCount = 0
    prclist = list();

    # test for dumper on port
    for serialport in ports:
        try:
            ser = serial.Serial( port = serialport, baudrate = 460800, bytesize = serial.EIGHTBITS, parity = serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE, timeout=0.1)
            ser.write(bytes("flash\r\n","utf-8"))
            response = ser.readline().decode("utf-8")
            if response == "thunder\r\n":
                #print(serialport)
                ser.close()
                newMultiArgs = list(multiArgs)
                newMultiArgs.append(serialport)
                prc = Popen(newMultiArgs) #, stdout=STDOUT, stderr=STDOUT)
                prclist.append(prc)
                umdCount += 1
            else:
                ser.close()
        except (OSError, serial.SerialException):
            pass

	# wait for all the UMD's to complete
    for prci in prclist:
        while prci.poll() is None:
            #print(".")
            time.sleep(1)
    
    print("Multi UMD Complete!")
