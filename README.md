# Universal Mega Dumper
The Universal Mega Dumper (UMD) is a game catridge read/writer project designed around a [Teensy++](https://www.pjrc.com/store/teensypp.html) microcontroller. The universality comes from the UMD's 
ability to support many different types of catridge connectors by having general purpose 16 bit data 
and 24 bit address paths along with a dozen control signals - all of which can be customized for each game
cartridge mode.

## Arduino Setup
### Download Teensyduino
Teensy is an Arduino compatible, but much better, microcontroller board. To use Teensy, head to the PJRC 
website and download the latest version of [Teensyduino](https://www.pjrc.com/teensy/td_download.html). Don't
install it yet!

Be sure to note the latest supported version of the Arduino IDE from the PJRC website as this is the version 
you will need to download from the Arduino website.

### Download and Install Arduino
Download the latest [Arduino IDE](https://www.arduino.cc/en/Main/Software) version supported by Teensyduino as
noted on the Teensy website. Extract the Arduino IDE. If you are running Linux like me you will have to make the
install.sh executable (chmod +x install.sh) in order to install once extracted.

### Install Teensyduino
Browse to the location where you downloaded Teensyduino. If you are using Linux, make the file executable (chmod +x)
and then run it. The installer will ask you to specify the directory where you extracted the Arduino IDE. Follow the
instructions to finalize the installtion.

### Linux udev Rules
If you are using Linux, you will need to perform this step. Create a file called **49-teensy.rules** with the following contents

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

## Python Setup
For linux users, python3 is more likely than not already installed in your
distro. UMD depends on pyserial, run the following commands to install it.

sudo apt install python3-pip
python3 -m pip install pyserial 
