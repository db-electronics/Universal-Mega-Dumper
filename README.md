# Universal Mega Dumper
The Universal Mega Dumper (UMD) is a game catridge read/writer project designed around a [Teensy++](https://www.pjrc.com/store/teensypp.html) 
microcontroller. The universality comes from the UMD's ability to support many different types of catridge connectors by having general 
purpose 16 bit data and 24 bit address paths along with a dozen control signals - all of which can be customized for each game
cartridge mode.

## UMD Adapter ID
Moving forward, each UMD adapter will contain an 8 bit shift register which the UMD firmware can use to identify the currently connected UMD Adapter and cartridge type.
* 0x01 - [NES](https://github.com/db-electronics/umd-nes-adapter-kicad)
* 0x02 - [Commodore 64](https://github.com/db-electronics/umd-c64-adapter-kicad)
* 0x03 - [SNES](https://github.com/db-electronics/umd-snes-adapter-kicad)

# Arduino Setup
## Download Teensyduino
Teensy is an Arduino compatible, but much better, microcontroller board. To use Teensy, head to the PJRC 
website and download the latest version of [Teensyduino](https://www.pjrc.com/teensy/td_download.html). Don't
install it yet!

Be sure to note the latest supported version of the Arduino IDE from the PJRC website as this is the version 
you will need to download from the Arduino website.

## Download and Install Arduino
Download the latest [Arduino IDE](https://www.arduino.cc/en/Main/Software) version supported by Teensyduino as
noted on the Teensy website. Extract the Arduino IDE. If you are running Linux like me you will have to make the
install.sh executable (chmod +x install.sh) in order to install once extracted.

## Install Teensyduino
Browse to the location where you downloaded Teensyduino. If you are using Linux, make the file executable (chmod +x)
and then run it. The installer will ask you to specify the directory where you extracted the Arduino IDE. Follow the
instructions to finalize the installtion.

## Linux udev Rules
If you are using linux, you will need to perform this step. Create a file called **49-teensy.rules** with the following contents

```
ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789B]?", ENV{ID_MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789A]?", ENV{MTP_NO_PROBE}="1"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789ABCD]?", MODE:="0666"
KERNEL=="ttyACM*", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="04[789B]?", MODE:="0666"
```
Next, copy this file to /etc/udev/rules.d/
```
sudo cp 49-teensy.rules /etc/udev/rules.d/
```

Now trigger udev to re-evaluate the rules
```
sudo udevadm trigger
```

If you have any issues with the Arduino IDE or teensyduino detecting the board, reboot.


# Arduino IDE Preferences
You can use the default Arduino library directory, which is ``~/Arduino/libraries``.  If you're ok with this, clone this and the required libraries there, and ignore the rest of this section.

If you don't want to use the default, open the Arduino IDE and go to File->Preferences.

In the "Sketchbook location" field, change it to the directory you plan to clone the UMD repository and dependent libraries.

For example:
```
/home/username/projects/umd
```

The ``libraries`` directory under this directory is where you should clone this repository and the required Arduino libraries.

# Python Setup
For linux users, python3 is more likely than not already installed in your distro. UMD depends on pyserial, run the following commands to install it.
```
sudo apt install python3-pip
python3 -m pip install pyserial 
```
# Install Arduino Libraries
UMD depends on a several Arduino libraries which need to be added to your Arduino/libraries folder explicitely. Installing these two libraries
will allow you to build the Teensy firmware.

## Arduino Serial Command
I have forked Arduino Serial Command and made a few modifications. Clone the Arduino Serial Command repo into your Arudino/libraries folder.

```
git clone https://github.com/db-electronics/ArduinoSerialCommand
```

## Serial Flash
Paul Stoffregen, the creator of Teensy, provides a very good SPI Serial Flash library with a rudimentary filesystem. Clone the Serial Flash
repo into your Arduino/libraries folder.

```
git clone https://github.com/PaulStoffregen/SerialFlash
```
You should have a directory layout like this:

```
libraries/ArduinoSerialCommand
libraries/SerialFlash
libraries/Universal-Mega-Dumper
```

# Program the teensy

  1. In the Arduino IDE, select Tools->Board->Teensy++2.0
  2. Open the sketch Universal-Mega-Dumper/Examples/Interface.ino
  3. Click the Verify (checkbox) button.
  4. Press the button on the teensy
     A teensy window should open showing that it's programming and rebooting.  If not, click the upload button.

# GUI front-end "Basic Option Builder" for umd.py

There's a very basic GUI program if you'd like to use it.

To run the GUI, you'll need appJar installed.

```
python3 -m pip install appJar
```

You may also need to install tk.

Then, run the gui from the same directory as umd.py

```
python3 gui.py
```
