 /*
    Title:          dbDumper.h
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
 
    This file is part of dbDumper.

    dbDumper is free software: you can redistribute it and/or modify
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

#ifndef dbDumper_h
#define dbDumper_h

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

class dbDumper
{
	public:
		dbDumper();
		bool dbDumper::detectCart()
		uint16_t dbDumper::getFlashId()

		//read
		uint16_t dbDumper::readWord(uint32_t);
		uint8_t dbDumper::readByte(uint32_t, uint8_t);

		//write
		void dbDumper::writeWord(uint32_t, uint16_t);
		void dbDumper::writeByte(uint32_t, uint8_t);
		
	private:
		void _latchAddress(uint32_t);

		//pin numbers
		static const uint8_t nLWR = 8;
		static const uint8_t nUWR = 9;
		static const uint8_t ALE_low = 26;
		static const uint8_t ALE_high = 27;
		static const uint8_t nRD = 25;
		static const uint8_t M07 = 24;
		static const uint8_t nSCE = 23;
		static const uint8_t nGCE = 22;
		static const uint8_t nPCE = 21;
		static const uint8_t nRST = 20;
		static const uint8_t nTIME = 19;
		static const uint8_t nGWR = 18;
		static const uint8_t nPCD = 38;
		static const uint8_t nGCD = 39;
		static const uint8_t nSCD = 40;
		static const uint8_t nPBtn = 44;
		static const uint8_t nLED = 45;
};

  uint8_t dataBuffer[1024];

#endif  //dbDumper_h

