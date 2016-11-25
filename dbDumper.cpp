/*******************************************************************//**
 * \file dbDumper.cpp
 * \author René Richard
 * \brief This program allows to read and write to various game cartridges including: Genesis, Coleco, SMS, PCE - with possibility for future expansion.
 *  
 * Target Hardware:
 * Teensy++2.0 with db Electronics TeensyDumper board rev >= 1.1
 * Arduino IDE settings:
 * Board Type  - Teensy++2.0
 * USB Type    - Serial
 * CPU Speed   - 16 MHz
 **********************************************************************/

 /*
 LICENSE
 
    This file is part of dbDumper.

    dbDumper is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    dbDumper is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with dbDumper.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Arduino.h"
#include "dbDumper.h"

/*******************************************************************//**
 * The constructor sets the mode using setMode() to undefined.
 **********************************************************************/
dbDumper::dbDumper() 
{
  	setMode(undefined);
}

/*******************************************************************//**
 * The resetCart() function issues a 250ms active-low reset pulse to
 * the pin specified by the variable _resetPin.
 * 
 * \warning setMode() must be called prior to using this function.
  **********************************************************************/
void dbDumper::resetCart()
{
	digitalWrite(_resetPin, LOW);
	delay(250);
	digitalWrite(_resetPin, HIGH);
}

/*******************************************************************//**
 * The detectCart() functions tests if the nCART line is pulled low
 * by a cartridge.
 **********************************************************************/
bool dbDumper::detectCart()
{
  	bool detect = false;

  	if (digitalRead(nCART) == LOW)
	{
		detect = true;
	}
	return detect;
}

/*******************************************************************//**
 * The setMode() function configures the Teensy IO properly for the
 * selected cartridge. The _mode and _resetPin variables store the
 * current mode and resetPin #s for later use by the firmware.
 **********************************************************************/
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
		case MD:
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
			resetCart();
			_mode = MD;

			break;
		case TG:
			/** \todo add pcengine pin mode */
			
			break;
		case CV:
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
			_mode = CV;
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

/*******************************************************************//**
 * The setMode() function returns the manufacturer and product ID from 
 * the Flash IC as a uint16_t. It automatically performs the correct
 * flash ID read sequence based on the currently selected mode.
 * 
 * \warning setMode() must be called prior to using this function.
 **********************************************************************/
uint16_t dbDumper::getFlashID()
{
  	uint16_t flashID = 0;

  	switch(_mode)
  	{
		//mx29f800 software ID detect word mode
		case MD:
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0090);
			flashID = readWord(0x0001);
			writeWord((uint16_t)0x0000, 0x00F0);
			_flashID = flashID;
			break;
		//mx29f800 software ID detect byte mode
		case TG:
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x90);
			flashID = (uint16_t)readByte(0x0002);
			writeByte((uint16_t)0x0000, 0xF0);
			_flashID = flashID;
			break;
		//SST39SF0x0 software ID detect
    	case CV:
			digitalWrite(COL_nBPRES, LOW);
			writeByte((uint16_t)0x5555,0xAA);
			writeByte((uint16_t)0x2AAA,0x55);
			writeByte((uint16_t)0x5555,0x90);
			
			flashID = (uint16_t)readByte(0x0000);
			flashID <<= 8;
			flashID |= (uint16_t)readByte(0x0001);
			
			//exit software ID
			writeByte((uint16_t)0x0000,0xF0);
			
			_flashID = flashID;
      		break;
		default:
      		flashID = 0xFFFF;
      	break;
  	}

  	return flashID;
}

/*******************************************************************//**
 * The setMode() function erases the entire chip. If the wait parameter
 * is true the function will block with toggle bit until the erase 
 * operation has completed. It will return the time in millis the
 * operation required to complete.
 * 
 * \warning setMode() must be called prior to using this function.
 **********************************************************************/
uint32_t dbDumper::eraseChip(bool wait)
{
	uint32_t startMillis;
	
  	switch(_mode)
  	{
		//mx29f800 chip erase word mode
		case MD:
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0080);
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0010);
			break;
		//mx29f800 chip erase byte mode
		case TG:
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x80);
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x10);
			break;
		//SST39SF0x0 chip erase
    	case CV:
			digitalWrite(COL_nBPRES, LOW);
			writeByte((uint16_t)0x5555, 0xAA);
			writeByte((uint16_t)0x2AAA, 0x55);
			writeByte((uint16_t)0x5555, 0x80);
			writeByte((uint16_t)0x5555, 0xAA);
			writeByte((uint16_t)0x2AAA, 0x55);
			writeByte((uint16_t)0x5555, 0x10);
      		break;
		default:
			break;
  	}
  	
  	// if wait parameter was specified, do toggle until operation is complete
  	if(wait)
  	{
		startMillis = millis();
		
		// wait for 4 consecutive toggle bit success reads before exiting
		while( toggleBit(4) != 4 );
		return ( millis() - startMillis );
		
	}else
	{
		return 0;
	}
}

/*******************************************************************//**
 * The readByte(uint32_t) function returns a byte read from 
 * a 24bit address.
 * 
 * \warning setMode() must be called prior to using this function.
 **********************************************************************/
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
  
	//read genesis odd bytes from the high byte of the bus
	switch(_mode)
	{
		case MD:
			if( (uint8_t)(address) & 0x01 )
			{
				readData = DATAINH;
			}else
			{
				readData = DATAINL;
			}
			break;
		case TG:
			readData = DATAINL;
			break;
		case CV:
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

/*******************************************************************//**
 * The readByteBlock function reads a block of bytes size blockSize into
 * *buf starting at the specified 24bit address.
 * 
 * \warning setMode() must be called prior to using this function.
 **********************************************************************/
void dbDumper::readByteBlock(uint32_t address, uint16_t blockSize)
{
	uint16_t i;

	for( i = 0 ; i < blockSize ; i++ )
	{
		_latchAddress(address);
		
		//set data bus to inputs
		DATAH_DDR = 0x00;
		DATAL_DDR = 0x00;
		
		// read the bus
		digitalWrite(nCE, LOW);
		digitalWrite(nRD, LOW);
		
		//read genesis odd bytes from the high byte of the bus
		switch(_mode)
		{
			case MD:
				if( (uint8_t)(address) & 0x01 )
				{
					buffer[i] = DATAINH;
				}else
				{
					buffer[i] = DATAINL;
				}
				break;
			case TG:
				buffer[i] = DATAINL;
				break;
			case CV:
				buffer[i] = DATAINL;
				break;
			default:
				buffer[i] = DATAINL;
				break;
		}
		
		digitalWrite(nCE, HIGH);
		digitalWrite(nRD, HIGH);
		
		address += 1;
	}
	//Serial.write( buffer, blockSize );
}

/*******************************************************************//**
 * The readWord(uint32_t) function returns a word read from 
 * a 24bit address.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning converts to little endian
 **********************************************************************/
uint16_t dbDumper::readWord(uint32_t address)
{
	//only genesis mode reads words

	uint16_t readData;

  	_latchAddress(address);

  	//set data bus to inputs
  	DATAH_DDR = 0x00;
  	DATAL_DDR = 0x00;

  	// read the bus
  	digitalWrite(nCE, LOW);
  	digitalWrite(nRD, LOW);
  
	//convert to little endian while reading
  	readData = (uint16_t)DATAINL;
  	readData <<= 8;
  	readData |= (uint16_t)(DATAINH & 0x00FF);
  
  	digitalWrite(nCE, HIGH);
  	digitalWrite(nRD, HIGH);

  	return readData;
}

/*******************************************************************//**
 * The readWordBlock function reads a block of bytes size blockSize into
 * buf starting at the specified 24bit address. The data is read as
 * big endian.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning Data read as big endian.
 **********************************************************************/
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
		//convert to little endian while reading
		buf[i] = DATAINH;
		buf[i+1] = DATAINL;
		digitalWrite(nCE, HIGH);
		digitalWrite(nRD, HIGH);
		
		address += 2;
	}
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 16bit
 * address. The upper 8 address bits (23..16) are not modified
 * by this function so this can be used to perform quicker successive
 * writes within a 64k boundary.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
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
	
#ifdef _DEBUG_DB
	Serial.print(F("w ")); 
	Serial.print(address,HEX);
	Serial.print(F(" : ")); 
	Serial.println(data,HEX);
#endif
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 24bit
 * address.
 * 
 * \warning setMode() must be called prior to using this function.
 **********************************************************************/
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

#ifdef _DEBUG_DB
	Serial.print(F("w ")); 
	Serial.print(address,HEX);
	Serial.print(F(" : ")); 
	Serial.println(data,HEX);
#endif
}

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 16bit
 * address. The upper 8 address bits (23..16) are not modified
 * by this function so this can be used to perform quicker successive
 * writes within a 64k boundary.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning upper 8 address bits (23..16) are not modified
 * \warning word is converted to big endian
 **********************************************************************/
void dbDumper::writeWord(uint16_t address, uint16_t data)
{
	_latchAddress(address);

	//set data bus to outputs
	DATAH_DDR = 0xFF;
	DATAL_DDR = 0xFF;

	//put word on bus
	DATAOUTH = (uint8_t)(data);
	DATAOUTL = (uint8_t)(data>>8);

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

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 16bit
 * address.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning word is converted to big endian
 **********************************************************************/
void dbDumper::writeWord(uint32_t address, uint16_t data)
{
	_latchAddress(address);

	//set data bus to outputs
	DATAH_DDR = 0xFF;
	DATAL_DDR = 0xFF;

	//put word on bus
	DATAOUTH = (uint8_t)(data);
	DATAOUTL = (uint8_t)(data>>8);

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

/*******************************************************************//**
 * The programByte function programs a byte into the flash array at a
 * 24bit address. If the wait parameter is true the function will wait 
 * for the program operation to complete using data polling before 
 * returning.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning Sector or entire IC must be erased prior to programming
 **********************************************************************/
void dbDumper::programByte(uint32_t address, uint8_t data, bool wait)
{
	uint8_t readBack = ~data;
	
  	switch(_mode)
  	{
		//MX29F800 program byte
		case TG:
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0xA0);
			writeByte(address, data);
			
			//use data polling to validate end of program cycle
			if(wait)
			{
				while(readBack != data)
				{
					readBack = readByte(address);
				}
			}
			break;
		//SST39SF0x0 program byte
		case CV:
			digitalWrite(COL_nBPRES, LOW);
			writeByte((uint16_t)0x5555, 0xAA);
			writeByte((uint16_t)0x2AAA, 0x55);
			writeByte((uint16_t)0x5555, 0xA0);
			
			writeByte(address, data);
			
			//use data polling to validate end of program cycle
			if(wait)
			{
				while(readBack != data)
				{
					//cast to uint16_t in case we got here
					readBack = readByte(address);
				}
			}
      		break;
		default:
			break;
  	}
}

/*******************************************************************//**
 * The programByte function programs a word into the flash array at a
 * 24bit address. If the wait parameter is true the function will wait 
 * for the program operation to complete using data polling before 
 * returning.
 * 
 * \warning setMode() must be called prior to using this function.
 * \warning Sector or entire IC must be erased prior to programming
 **********************************************************************/
void dbDumper::programWord(uint32_t address, uint16_t data, bool wait)
{
	uint16_t readBack = ~data;
	
  	switch(_mode)
  	{
		//MX29F800 program word
		case MD:
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x00A0);
			writeByte(address, data);
			
			//use data polling to validate end of program cycle
			if(wait)
			{
				while(readBack != data)
				{
					readBack = readWord(address);
				}
			}
			break;
		default:
			break;
  	}
}

/*******************************************************************//**
 * The _latchAddress function latches a 24bit address to the cartridge
 * \warning incompatible with Colecovision mode
 **********************************************************************/
inline void dbDumper::_latchAddress(uint32_t address)
{
	uint8_t addrh,addrm,addrl;
	
	// set the coleco address bits if in coleco mode
	if( _mode == CV)
	{
		_colAddrBitsSet(address);
	}
	
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

/*******************************************************************//**
 * The _latchAddress function latches a 16bit address to the cartridge.
 * In Colecovision mode the 4 nCE lines are automatically handled wrt
 * the address range.
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
inline void dbDumper::_latchAddress(uint16_t address)
{
	uint8_t addrm,addrl;
	
	// set the coleco address bits if in coleco mode
	if( _mode == CV)
	{
		_colAddrBitsSet(address);
	}
	
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

void dbDumper::eraseSector(uint16_t sectorAddress)
{
  	switch(_mode)
  	{
		case MD:
			//mx29f800 chip erase word mode
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0080);
			writeWord((uint16_t)0x0555, 0x00AA);
			writeWord((uint16_t)0x02AA, 0x0055);
			writeWord((uint16_t)0x0555, 0x0010);
			break;
		case TG:
			//mx29f800 chip erase byte mode
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x80);
			writeByte((uint16_t)0x0AAA, 0xAA);
			writeByte((uint16_t)0x0555, 0x55);
			writeByte((uint16_t)0x0AAA, 0x10);
			break;
    	case CV:
			//SST39SF0x0 chip erase
			digitalWrite(COL_nBPRES, LOW);
			writeByte((uint16_t)0x5555, 0xAA);
			writeByte((uint16_t)0x2AAA, 0x55);
			writeByte((uint16_t)0x5555, 0x80);
			writeByte((uint16_t)0x5555, 0xAA);
			writeByte((uint16_t)0x2AAA, 0x55);

			//sector address comes here
			writeByte((uint16_t)0x5555, 0x10);
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
		//mx29f800 toggle bit on bit 6
		case MD:
			
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
		case TG:
		
		//SST39SF0x0 toggle bit on bit 6
    	case CV:
			
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

void dbDumper::_colAddrBitsSet(uint16_t address)
{
	//determine which address range to use, look at the four MS bits (A16..A0)
	uint8_t smallRange = (uint8_t)(address >> 13);
	
#ifdef _DEBUG_DB	
	Serial.println(smallRange,HEX);
#endif

	switch(smallRange)
	{
		// bbba aaaa aaaa aaaa
		// 1110 0000 0000 0000
		case 0x00:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x01:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x02:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x03:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x04:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x05:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x06:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x07:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			break;
		default:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
	}
}

void dbDumper::_colAddrBitsSet(uint32_t address)
{
	//determine which address range to use, look at the four MS bits (A16..A0)
	uint8_t smallRange = (uint8_t)(address >> 13);

#ifdef _DEBUG_DB	
	Serial.println(smallRange,HEX);
#endif

	switch(smallRange)
	{
		// 000b bbba aaaa aaaa aaaa
		// 0001 1110 0000 0000 0000
		case 0x00:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x01:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x02:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x03:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x04:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x05:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x06:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x07:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x08:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x09:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x0A:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x0B:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x0C:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x0D:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, HIGH);
			break;
		case 0x0E:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, LOW);
			break;
		case 0x0F:
			digitalWrite(COL_nE000, HIGH);
			digitalWrite(COL_nC000, HIGH);
			digitalWrite(COL_nA000, HIGH);
			digitalWrite(COL_n8000, HIGH);
			break;
		default:
			digitalWrite(COL_nE000, LOW);
			digitalWrite(COL_nC000, LOW);
			digitalWrite(COL_nA000, LOW);
			digitalWrite(COL_n8000, LOW);
			break;
	}
}

/*******************************************************************//**
 * The _convColecoAddr function takes the real ROM image address and
 * converts it to an address compatible with the db Coleco carts since
 * these carts require each 8KB segment of the ROM to be remapped for
 * a reduced parts count address decoding scheme.
 **********************************************************************/
uint32_t dbDumper::convColecoAddr(uint32_t address)
{
	uint16_t range;
	
	//determine which address range to use, look at the two MS bits (32KB)
	range = address & 0x6000;
	address &= 0x1FFF;
	switch(range)
	{
		// 000b bbba aaaa aaaa aaaa
		// 0001 1100 0000 0000 0000
		case 0x0000:
			return ( 0x0001C000 | (uint32_t)address);
			break;
		// 000b bbba aaaa aaaa aaaa	
		// 0001 1010 0000 0000 0000
		case 0x2000:
			return ( 0x0001A000 | (uint32_t)address);
			break;
		// 000b bbba aaaa aaaa aaaa	
		// 0001 0110 0000 0000 0000
		case 0x4000:
			return ( 0x00016000 | (uint32_t)address);
			break;
		// 000b bbba aaaa aaaa aaaa	
		// 0000 1110 0000 0000 0000
		case 0x6000:
			return ( 0x00007000 | (uint32_t)address);
			break;
		default:
			break;
	}
	return 0;
}

