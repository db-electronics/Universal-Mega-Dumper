 /*
    Title:          dbDumper.cpp
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

dbDumper::dbDumper() 
{

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
			_colPinMode();
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
			_mode = undefined;
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
	if( _mode == coleco )
	{
		_colAddrRangeSet((uint16_t)address);
	}
	//set data bus to inputs
	DATAH_DDR = 0x00;
	DATAL_DDR = 0x00;

	// read the bus
	digitalWrite(nCE, LOW);
	digitalWrite(nRD, LOW);
  
	//read genesis odd bytes from the high byte of the bus
	switch(_mode)
	{
		case genesis:
			if( (uint8_t)(address) & 0x01 )
			{
				readData = DATAINH;
			}else
			{
				readData = DATAINL;
			}
			break;
		case coleco:
			_colAddrRangeSet((uint16_t)address);
			readData = DATAINL;
			break;
		default:
			readData = DATAINL;
			break;
	}
  
	digitalWrite(nCE, HIGH);
	digitalWrite(nRD, HIGH);

	return readData;
}

uint8_t dbDumper::readByte(uint16_t address)
{
	uint8_t readData;
	
	_latchAddress(address);
	if( _mode == coleco )
	{
		_colAddrRangeSet(address);
	}

	//set data bus to inputs
	DATAH_DDR = 0x00;
	DATAL_DDR = 0x00;

	// read the bus
	digitalWrite(nCE, LOW);
	digitalWrite(nRD, LOW);
  
	//read genesis odd bytes from the high byte of the bus
	switch(_mode)
	{
		case genesis:
			if( (uint8_t)(address) & 0x01 )
			{
				readData = DATAINH;
			}else
			{
				readData = DATAINL;
			}
			break;
		case coleco:
			readData = DATAINL;
			break;
		default:
			readData = DATAINL;
			break;
	}
  
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


void dbDumper::readWordBlock(uint32_t address, uint8_t * buf, uint16_t blockSize)
{
	uint16_t i;

	for( i = 0 ; i < blockSize ; i += 2 )
	{
		_latchAddress(address);
		
		//set data bus to inputs
		DATAH_DDR = 0x00;
		DATAL_DDR = 0x00;
		
		// read the bus
		digitalWrite(nCE, LOW);
		digitalWrite(nRD, LOW);
		buf[i] = DATAINH;
		buf[i+1] = DATAINL;
		digitalWrite(nCE, HIGH);
		digitalWrite(nRD, HIGH);
		
		address += 2;
	}
}

void dbDumper::readByteBlock(uint32_t address, uint8_t * buf, uint16_t blockSize)
{
	uint16_t i;

	for( i = 0 ; i < blockSize ; i += 1 )
	{
		_latchAddress(address);
		
		//set data bus to inputs
		DATAH_DDR = 0x00;
		DATAL_DDR = 0x00;
		
		// read the bus
		digitalWrite(nCE, LOW);
		digitalWrite(nRD, LOW);
		buf[i] = DATAINL;
		digitalWrite(nCE, HIGH);
		digitalWrite(nRD, HIGH);
		
		address += 1;
	}
}

inline void dbDumper::_latchAddress(uint32_t address)
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

inline void dbDumper::_latchAddress(uint16_t address)
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
  	uint16_t flashID = 0;

  	switch(_mode)
  	{
		case genesis:
			//mx29f800 software ID detect word mode
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0090);
			flashID = readWord((uint16_t)0x0001);
			writeWord((uint16_t)0x0000, 0x00F0);
			break;
		case pcengine:
			//mx29f800 software ID detect byte mode
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x90);
			flashID = (uint16_t)readByte((uint16_t)0x0002);
			writeByte((uint16_t)0x0000, 0xF0);
			break;
    	case coleco:
			//SST39SF0x0 software ID detect

			_colSoftwareIDEntry();
			
			flashID = (uint16_t)readByte((uint16_t)0x0000);
			flashID <<= 8;
			flashID |= (uint16_t)readByte((uint16_t)0x0001);
			_flashID = flashID;

			_colSoftwareIDExit();

      		break;
		default:
      		flashID = 0xFFFF;
      	break;
  	}

  	return flashID;
}

void dbDumper::chipErase()
{
  	switch(_mode)
  	{
		case genesis:
			//mx29f800 chip erase word mode
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0080);
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0010);
			break;
		case pcengine:
			//mx29f800 chip erase byte mode
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x80);
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x10);
			break;
    	case coleco:
			//SST39SF0x0 chip erase
			digitalWrite(COL_nBPRES, LOW);
			digitalWrite(COL_A16, LOW);
			digitalWrite(COL_A15, LOW);
			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, LOW);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0xAA);

			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, HIGH);
			writeByte((uint16_t)0x2AAA, 0x55);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0x80);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0xAA);
			
			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, HIGH);
			writeByte((uint16_t)0x2AAA, 0x55);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0x10);

			digitalWrite(COL_A16, LOW);
			digitalWrite(COL_A15, LOW);
			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, LOW);
      		break;
		default:
			break;
  	}
}

uint8_t dbDumper::toggleBit(uint8_t attempts)
{
	uint8_t retValue = 0;
	uint8_t i;
	
  	switch(_mode)
  	{
		case genesis:
			//mx29f800 toggle bit on bit 6
			uint16_t read16Value, old16Value;
			
			//first read should always be a 1 according to datasheet
			old16Value = readWord((uint16_t)0x0000) & 0x0040;
			
			for( i=0; i<attempts; i++ )
			{
				//successive reads compare this read to the previous one for toggle bit
				read16Value = readWord((uint16_t)0x0000) & 0x0040;
				if( old16Value == read16Value )
				{
					retValue += 1;
				}else
				{
					retValue = 0;
				}
				old16Value = read16Value;
			}
			break;
		case pcengine:
    	case coleco:
			//SST39SF0x0 toggle bit on bit 6
			uint8_t readValue, oldValue;
			
			//first read should always be a 1 according to datasheet
			oldValue = readByte((uint16_t)0x0000) & 0x40;
			
			for( i=0; i<attempts; i++ )
			{
				//successive reads compare this read to the previous one for toggle bit
				readValue = readByte((uint16_t)0x0000) & 0x40;
				if( oldValue == readValue )
				{
					retValue += 1;
				}else
				{
					retValue = 0;
				}
				oldValue = readValue;
			}
      		break;
		default:
			break;
  	}
  	return retValue;
}

void dbDumper::programByte(uint16_t address, uint8_t data)
{
  	switch(_mode)
  	{
		case pcengine:
			//MX29F800 program byte
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0xA0);
			writeByte(address, data);
			
			break;
    	case coleco:
			//SST39SF0x0 program byte
			digitalWrite(COL_nBPRES, LOW);
			digitalWrite(COL_A16, LOW);
			digitalWrite(COL_A15, LOW);
			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, LOW);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0xAA);

			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, HIGH);
			writeByte((uint16_t)0x2AAA, 0x55);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0xA0);
			
			//determine which address range to use, look at the two MS bits
			_colAddrRangeSet(address);
			
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0xA0);
			writeByte(address, data);
      		break;
      		
		default:
			break;
  	}
}

void dbDumper::programByte(uint32_t address, uint8_t data)
{
  	switch(_mode)
  	{
		case pcengine:
			//MX29F800 program byte
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0xA0);
			writeByte(address, data);
			break;
		case coleco:
			//SST39SF0x0 program byte
			digitalWrite(COL_nBPRES, LOW);
			digitalWrite(COL_A16, LOW);
			digitalWrite(COL_A15, LOW);
			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, LOW);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0xAA);

			digitalWrite(COL_A14, LOW);
			digitalWrite(COL_A13, HIGH);
			writeByte((uint16_t)0x2AAA, 0x55);

			digitalWrite(COL_A14, HIGH);
			digitalWrite(COL_A13, LOW);
			writeByte((uint16_t)0x5555, 0xA0);
			
			//determine which address range to use, look at the two MS bits
			_colAddrRangeSet((uint16_t)address);
			
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0xA0);
			writeByte((uint16_t)address, data);
      		break;
		default:
			break;
  	}
}

void dbDumper::programWord(uint32_t address, uint16_t data)
{
  	switch(_mode)
  	{
		case genesis:
			//MX29F800 program byte
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x00A0);
			writeByte(address, data);
			break;
		default:
			break;
  	}
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

