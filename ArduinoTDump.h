 /*
    Title:          ArduinoTDump.h
    Author:         René Richard
    Description:
        This library allows to read and write to various game cartridges
        including: Genesis, SMS, PCE - with possibility for future
        expansion.
    Target Hardware:
        Teensy++2.0 with db Electronics TeensyDumper board
    Arduino IDE settings:
        Board Type  - Teensy++2.0
        USB Type    - Serial
        CPU Speed   - 16 MHz

 LICENSE
 
    This file is part of ArduinoTDump.

    ArduinoTDump is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ArduinoTDump_h
#define ArduinoTDump_h

#define DEBUG         1

  //define pins
#define DATAOUTH      PORTD
#define DATAOUTL      PORTC
#define DATAINH       PIND
#define DATAINL       PINC
#define DATAH_DDR     DDRD
#define DATAL_DDR     DDRC

#define GENESIS       'G'
#define SMS           'S'
#define PCE           'P'

class ArduinoTDump
{
	public:
		ArduinoTDump();
		bool ArduinoTDump::detectCart()
		unsigned int ArduinoTDump::getFlashId()

		//read
		unsigned int ArduinoTDump::readWord(unsigned long);
		unsigned char ArduinoTDump::readByte(unsigned long, char);

		//write
		void ArduinoTDump::writeWord(unsigned long, unsigned int);
		void ArduinoTDump::
		
	private:
		void latchAddress(unsigned long);

		//pin numbers
		const int nLWR = 8;
		const int nUWR = 9;
		const int ALE_low = 26;
		const int ALE_high = 27;
		const int nRD = 25;
		const int M07 = 24;
		const int nSCE = 23;
		const int nGCE = 22;
		const int nPCE = 21;
		const int nRST = 20;
		const int nTIME = 19;
		const int nGWR = 18;
		const int nPCD = 38;
		const int nGCD = 39;
		const int nSCD = 40;
		const int nPBtn = 44;
		const int nLED = 45;
};

  char dataBuffer[1024];

#endif  //ArduinoTDump_h

