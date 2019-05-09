#!/bin/sh 

arduino_cmd=$(which arduino 2>/dev/null)
arduino_dbg_cmd=$(which arduino_debug 2>/dev/null)

if [ "$arduino_dbg_cmd" != "" ]; then
    arduino_cmd="$arduino_dbg_cmd"
fi

if [ "$arduino_cmd" == "" ]; then
    echo "Unable to find arduino CLI.  Add your arduino install path's bin directory to your path."
else 
    while getopts ":vu" opt; do
	    case ${opt} in
	    v) 
                "$arduino_cmd" --verify --board teensy:avr:teensypp2 Sketch.ino
		exit $?
                ;;
            u)
                "$arduino_cmd" --upload --board teensy:avr:teensypp2 Sketch.ino
		exit $?
                ;;
        esac
    done
fi

echo "Usage: $1 [-v] [-u]"
printf "\t-v verify"
printf "\t-u upload"
exit 1 
