 /*
    Title:          coleco.cpp
    Author:         René Richard
    Description:
        This library allows to read and write to various game cartridges
        including: Genesis, SMS, PCE - with possibility for future
        expansion.
    Target Hardware:
        Teensy++2.0 with db Electronics TeensyDumper board rev 1.1 or greater
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

#include "Arduino.h"
#include "dbDumper.h"

void dbDumper::_colSoftwareIDEntry()
{
	digitalWrite(COL_nBPRES, LOW);
	digitalWrite(COL_A16, LOW);
	digitalWrite(COL_A15, LOW);
	digitalWrite(COL_A14, LOW);
	digitalWrite(COL_A13, LOW);

	digitalWrite(COL_A14, HIGH);
	digitalWrite(COL_A13, LOW);
	writeByte((uint16_t)0x5555,0xAA);

	digitalWrite(COL_A14, LOW);
	digitalWrite(COL_A13, HIGH);
	writeByte((uint16_t)0x2AAA,0x55);

	digitalWrite(COL_A14, HIGH);
	digitalWrite(COL_A13, LOW);
	writeByte((uint16_t)0x5555,0x90);

	digitalWrite(COL_A16, LOW);
	digitalWrite(COL_A15, LOW);
	digitalWrite(COL_A14, LOW);
	digitalWrite(COL_A13, LOW);
}

void dbDumper::_colPinMode()
{
	pinMode(COL_nBPRES, OUTPUT);
	digitalWrite(COL_nBPRES, LOW);
	pinMode(COL_nE000, OUTPUT);
	digitalWrite(COL_nE000, LOW);
	pinMode(COL_nC000, OUTPUT);
	digitalWrite(COL_nC000, LOW);
	pinMode(COL_nA000, OUTPUT);
	digitalWrite(COL_nA000, LOW);
	pinMode(COL_n8000, OUTPUT);
	digitalWrite(COL_n8000, LOW);

	_resetPin = 45; //unused with coleco
	_mode = coleco;
}

void dbDumper::_colAddrRangeSet(uint16_t address)
{
	uint16_t range;
	
	//determine which address range to use, look at the two MS bits
	range = address & 0x6000;
	switch(range)
	{
		case 0x0000:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			
			break;
		case 0x2000:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			
			break;
		case 0x4000:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			
			break;
		case 0x6000:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			
			break;
		default:
			break;
	}
}

void dbDumper::_colSoftwareIDExit()
{
	writeByte((uint16_t)0x0000,0xF0);
}
