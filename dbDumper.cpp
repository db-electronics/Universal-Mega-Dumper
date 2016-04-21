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

  	setMode(undefined);

}

void dbDumper::resetCart(uint8_t)
{
	digitalWrite(_resetPin, LOW);
	delay(250);
	digitalWrite(_resetPin, HIGH);
}

void dbDumper::setMode(eMode mode)
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
		case coleco:
			
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

			break;
		case genesis:
			
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

			_resetPin = GEN_nVRES;
			resetCart(_resetPin);
			_mode = genesis;

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
	//only genesis mode reads word for now

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

uint16_t dbDumper::readWord(uint16_t address)
{
	//only genesis mode reads word for now

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

uint8_t dbDumper::readByte(uint16_t address)
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

void dbDumper::writeWord(uint16_t address, uint16_t data)
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

void dbDumper::writeByte(uint16_t address, uint8_t data)
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
  addrm = (uint8_t)(address>>8 & 0xFF);
  addrh = (uint8_t)(address>>16 & 0xFF);

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

void dbDumper::_latchAddress(uint16_t address)
{
  uint8_t addrm,addrl;
  //separate address into 2 bytes for address latches
  addrl = (uint8_t)(address & 0xFF);
  addrm = (uint8_t)(address>>8 & 0xFF);

  //set data to outputs
  DATAH_DDR = 0xFF;
  DATAL_DDR = 0xFF;

  //put low and mid address on bus and latch it
  DATAOUTH = addrm;
  DATAOUTL = addrl;
  digitalWrite(ALE_low, HIGH);
  digitalWrite(ALE_low, LOW);

}

uint16_t dbDumper::getFlashID()
{

  	uint16_t flashID;

  	switch(_mode)
  	{
    	case coleco:
			//SST39SF0x0 software ID detect

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
			flashID = (uint16_t)readByte((uint16_t)0x0001);

			writeByte((uint16_t)0x0000,0xF0);

      		break;
		default:
      		flashID = 0xFFFF;
      	break;
  	}

  	return flashID;
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

