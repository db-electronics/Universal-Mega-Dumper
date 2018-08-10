#!/usr/bin/env python3
# -*- coding: utf-8 -*-
########################################################################
# \file gui.py
# \author Brad Trimby
# \brief This program is a front-end for umd.py which allows to read and
#        write to various game cartridges including: Genesis, Coleco,
#        SMS, PCE - with possibility for future expansion.
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

from appJar import gui
import sys
import subprocess

# Event handler for when the run button is pressed.
# De-bounce the Run button so multiple runs don't happen,
#  and start the runUmd function in another thread
def runButton(button):
    app.queueFunction(app.setButtonState,"Run UMD","disabled")
    app.queueFunction(app.clearAllTextAreas)
    app.thread(runUmd)
    app.queueFunction(app.setButtonState,"Run UMD","normal")

# Should usually be run in another thread.  Builds argument list and calls python to run umd.py.
# This could be improved to run the code directly in-process but for now this was the easiest way
# to get a basic GUI up.
def runUmd():
    # Queuing gui-updates is recommended when using blocking I/O or when running from another thread.
    app.queueFunction(app.setStatusbarBg, "red",0)
    app.queueFunction(app.setStatusbar, "Running...",0)

    args = [ sys.executable,"umd.py","--mode", modes[app.getOptionBox("Mode")],
              "--size", app.getEntry("Size")+ (app.getOptionBox("unit") if app.getOptionBox("unit") is not None else "")]
    rwmode = app.getRadioButton("rwmode")
    ioOption = app.getOptionBox("I/O Option")
    if rwmode == "Read":
        args.append("--rd")
        args.append(ioOption)
    elif rwmode == "Clear":
        args.append("--clr")
        args.append(ioOption)
    elif rwmode == "Checksum":
        args.append("--checksum")
    else:
        args.append("--wr")
        args.append(ioOption)
    dat = app.getEntry("Datfile")
    outfile = app.getEntry("Output file")
    if dat !="":
        args.append("--dat")
        args.append(dat)
    if outfile !="":
        args.append("--file")
        args.append(outfile)
    args.append("--addr")
    args.append(app.getEntry("Start"))

    #app.infoBox("Test"," ".join(args))


    output = ""
    proc = subprocess.run(args, universal_newlines=True, stdout=subprocess.PIPE)
    for line in proc.stdout.split("\n"):
        if not line.startswith("Percent"):
            output = output + line + "\n"
    app.queueFunction(app.setTextArea, "Output",output)
    app.queueFunction(app.setStatusbarBg, "lightgray",0)
    app.queueFunction(app.setStatusbar, "Done",0)

# Event handler for when the read-write mode radio buttons change.
def rwmodeChanged(rb):
    rwmode = app.getRadioButton(rb)
    app.changeOptionBox("I/O Option",ioOptions[rwmode],0)


if __name__ == '__main__':

    # Cart mode map from label to umd argument value.
    modes = { "MegaDrive/Genesis": "gen",
              "Master System": "sms",
              "PC Engine": "pce",
              "TurboGrafx 16":"tg16",
              "SNES (HI)":"snes",
              "SNES (LO)":"sneslo" }

    # These options are swapped in and out of the io options drop-down when the corresponding
    # radio button is changed.
    ioOptions = { "Read": [ "rom", "header", "save", "bram", "fid", "sfid", "sf",
                           "sflist", "byte", "word","sbyte", "sword" ],
                  "Write": [ "rom", "save", "bram", "sf" ],
                  "Clear": [ "rom", "rom2", "save", "bram", "sf" ],
                  "Checksum": [""]}

    with gui("Universal Mega Dumper BOB (Basic Option Builder)") as app:
        app.setSize("640x600")
        app.setGuiPadding(10)
        app.setStretch("both")
        app.addStatusbar(fields=1)
        app.setStatusbarBg("lightgray",0)
        app.setStatusbar("",0)

        # The GUI is currently laid out in a 3-column grid.
        # Grid is zero-indexed, the following widgets are intended to be
        # in-order of where they show up on the grid, but be careful as contents may
        #  have shifted since departure.

        app.addLabel("ModeLbl", "Mode", 0, 0)
        app.setLabelAnchor("ModeLbl","e")
        app.addOptionBox("Mode", sorted(modes.keys()), 0, 1, colspan=2)

        app.addRadioButton("rwmode","Read", 1, 1)
        app.setRadioButton("rwmode","Read")
        app.addRadioButton("rwmode","Write", 1, 2)
        app.addRadioButton("rwmode","Clear", 2, 2)
        app.addRadioButton("rwmode","Checksum", 2, 1)
        app.setRadioButtonChangeFunction("rwmode", rwmodeChanged)

        app.addLabel("ReadOptLbl", "I/O Option", 3, 0)
        app.setLabelAnchor("ReadOptLbl","ne")
        app.addOptionBox("I/O Option", ioOptions["Read"],3,1,colspan=2)

        app.addLabel("SizeLbl", "Size",4,0)
        app.setLabelAnchor("SizeLbl","e")
        app.addEntry("Size",4,1)
        app.setEntry("Size","0")
        app.addOptionBox("unit",["","KB","MB","Kb","Mb"],4,2)

        app.addLabel("StartLbl","Starting Address", 5,0)
        app.setLabelAnchor("StartLbl","e")
        app.addEntry("Start",5,1, colspan=2)
        app.setEntry("Start","0")

        app.addLabel("OutputFileLbl", "Output file", 6, 0)
        app.setLabelAnchor("OutputFileLbl", "e")
        app.addFileEntry("Output file", 6, 1, colspan=2)

        app.addLabel("DatfileLbl", "Datfile", 7, 0)
        app.setLabelAnchor("DatfileLbl","e")
        app.addFileEntry("Datfile",7,1, colspan=2)
        app.addButtons(["Run UMD"], [runButton], 8,2, colspan=2)
        app.addScrolledTextArea("Output", colspan=3)


