 /*
    Title:          dbDumper.cpp
    Author:         René Riuint8_td
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

dbDumper::dbDumper() {

  	setMode(0);

}

void dbDumper::resetCart(uint8_t)
{
	digitalWrite(_resetPin, LOW);
	delay(250);
	digitalWrite(_resetPin, HIGH);
}

void dbDumper::setMode(uint16_t mode)
{
	//Dataport as inputs, use port access for performance on these
	DATAH_DDR = 0x00;
	DATAL_DDR = 0x00;

	//74HC373 latch enable input is active high, default to low
  	pinMode(ALE_low, OUTPUT);
  	digitalWrite(ALE_low, LOW);
  	pinMode(ALE_high, OUTPUT);
  	digitalWrite(ALE_high, LOW);

	//global outputs signal default to high
	pinMode(nWR, OUTPUT);
  	digitalWrite(nWR, HIGH);
  	pinMode(nRD, OUTPUT);
  	digitalWrite(nRD, HIGH);
	pinMode(nCE, OUTPUT);
  	digitalWrite(nRD, HIGH);

  	//cartridge detect
  	pinMode(nCART, INPUT_PULLUP);

	//LED and pushbutton
	pinMode(nLED, OUTPUT);
	digitalWrite(nLED, HIGH);
	pinMode(nPB, INPUT);

	switch(mode)
	{
		case GENESIS:
			_resetPin = GEN_nVRES;
			pinMode(GEN_SL1, INPUT);
		  	pinMode(GEN_SR1, INPUT);
		  	pinMode(GEN_nDTACK, OUTPUT);
			digitalWrite(GEN_nDTACK, HIGH);
		  	pinMode(GEN_nCAS2, OUTPUT);
			digitalWrite(GEN_nCAS2, HIGH);
			pinMode(GEN_nVRES, OUTPUT);
			digitalWrite(GEN_nVRES, HIGH);
			pinMode(GEN_nLWR, OUTPUT);
			digitalWrite(GEN_nLWR, HIGH);
			pinMode(GEN_nUWR, OUTPUT);
			digitalWrite(GEN_nUWR, HIGH);
			pinMode(GEN_nTIME, OUTPUT);
			digitalWrite(GEN_nTIME, HIGH);
			resetCart(_resetPin);
			break;	
		default:
			//control signals default to all inputs
		  	pinMode(CTRL0, INPUT);
		  	pinMode(CTRL1, INPUT);
		  	pinMode(CTRL2, INPUT);
		  	pinMode(CTRL3, INPUT);
			pinMode(CTRL4, INPUT);
			pinMode(CTRL5, INPUT);
			pinMode(CTRL6, INPUT);
			pinMode(CTRL7, INPUT);
			break;
	}
}

uint16_t dbDumper::readWord(uint32_t address)
{
  uint16_t readData;

  _latchAddress(address);

  //set data bus to inputs
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;

  // read the bus
  digitalWrite(nCE, LOW);
  digitalWrite(nRD, LOW);
  
  readData = (uint16_t)DATAINH;
  readData <<= 8;
  readData |= (uint16_t)(DATAINL & 0x00FF);
  
  digitalWrite(nCE, HIGH);
  digitalWrite(nRD, HIGH);

  return readData;
}

uint8_t dbDumper::readByte(uint32_t address)
{
  uint8_t readData;

  _latchAddress(address);

  //set data bus to inputs
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;

  // read the bus
  digitalWrite(nCE, LOW);
  digitalWrite(nRD, LOW);
  
  readData = DATAINL;
  
  digitalWrite(nCE, HIGH);
  digitalWrite(nRD, HIGH);

  return readData;
}

void dbDumper::writeWord(uint32_t address, uint16_t data)
{
  _latchAddress(address);

  //set data bus to outputs
  DATAH_DDR = 0xFF;
  DATAL_DDR = 0xFF;

  //put word on bus
  DATAOUTL = (uint8_t)(data);
  DATAOUTH = (uint8_t)(data>>8);

  // write to the bus
  digitalWrite(nCE, LOW);
  digitalWrite(nWR, LOW);
  delayMicroseconds(1);
  digitalWrite(nWR, HIGH);
  digitalWrite(nCE, HIGH);
  
  //set data bus to inputs
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;
}

void dbDumper::writeByte(uint32_t address, uint8_t data)
{
  _latchAddress(address);

  //set data bus to outputs
  DATAH_DDR = 0xFF;
  DATAL_DDR = 0xFF;

  //put word on bus
  DATAOUTL = data;

  // write to the bus
  digitalWrite(nCE, LOW);
  digitalWrite(nWR, LOW);
  delayMicroseconds(1);
  digitalWrite(nWR, HIGH);
  digitalWrite(nCE, HIGH);
  
  //set data bus to inputs
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;
}

/*
void dbDumper::readBlock(uint32_t address, uint32_t blockSize)
{
  uint16_t i;

  for(i=0 ; i < blockSize ; i+=2)
  {
    _latchAddress(address);
    //set data bus to inputs
    DATAH_DDR = 0x00;
    DATAL_DDR = 0x00;
    // read the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nRD, LOW);
    dataBuffer[i] = DATAINH;
    dataBuffer[i+1] = DATAINL;
    digitalWrite(nCE, HIGH);
    digitalWrite(nRD, HIGH);
    address += 2;
  }
}
*/

void dbDumper::_latchAddress(uint32_t address)
{
  uint8_t addrh,addrm,addrl;
  //separate address into 3 bytes for address latches
  addrl = (uint8_t)(address & 0xFF);
  addrm = (uint16_t)(address>>8 & 0xFF);
  addrh = (uint16_t)(address>>16 & 0xFF);

  //set data to outputs
  DATAH_DDR = 0xFF;
  DATAL_DDR = 0xFF;

  //put low and mid address on bus and latch it
  DATAOUTH = addrm;
  DATAOUTL = addrl;
  digitalWrite(ALE_low, HIGH);
  digitalWrite(ALE_low, LOW);

  //put high address on bus and latch it
  DATAOUTH = 0x00;
  DATAOUTL = addrh;
  digitalWrite(ALE_high, HIGH);
  digitalWrite(ALE_high, LOW);
}

uint16_t dbDumper::getFlashId()
{
/*
  uint16_t flashID;
  switch(type)
  {
    case 'M': //macronix
      writeWordGen(0x555,0xAA);
      writeWordGen(0x2AA,0x55);
      writeWordGen(0x555,0x90);
      flashID = readWordGen(0x01);
      break;
    default:
      flashID = 0xFFFF;
      break;
  }
*/
  return 0;
}

bool dbDumper::detectCart()
{
  	bool detect = false;

  	if (digitalRead(nCART) == LOW)
	{
		detect = true;
	}
	return detect;
}

