# Universal Mega Dumper
The Universal Mega Dumper (UMD) is a game catridge read/writer project
designed around a Teensy++ 2 microcontroller. The universality comes
from the UMD's ability to support many different types of catridge connectors
by having general purpose 16 bit data and 24 bit address paths along with
a dozen control signals - all of which can be customized for each game
cartridge mode.

## Arduino Setup
Teensy is an Arduino compatible, but much better, microcontroller board.
To use Teensy, head to the PJRC website and download the latest version of
Teensyduino. Be sure to note the latest supported version of the Arduino IDE
from the PJRC website as this is the version you will need to download
from the Arduino website. Follow all instructions to install the Arduino IDE
and the Teensyduino add-on.

todo - cprules

## Python Setup
For linux users, python3 is more likely than not already installed in your
distro. UMD depends on pyserial, run the following commands to install it.

sudo apt install python3-pip
python3 -m pip install pyserial 
