/*******************************************************************//**
 *  \file umdv1.cpp
 *  \author René Richard
 *  \brief This program allows to read and write to various game cartridges. 
 *         The UMD base class handles all generic cartridge operations, console
 *         specific operations are handled in derived classes.
 *
 *  \copyright This file is part of Universal Mega Dumper.
 *
 *   Universal Mega Dumper is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Universal Mega Dumper is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Universal Mega Dumper.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Arduino.h"
#include "umdv1.h"

/*******************************************************************//**
 * The constructor
 **********************************************************************/
/*static*/ void umdv1::initialize() 
{
    SET_DATABUS_TO_INPUT();

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

    //All control signals default to input, you know, for safety
    pinMode(CTRL0, INPUT);
    pinMode(CTRL1, INPUT);
    pinMode(CTRL2, INPUT);
    pinMode(CTRL3, INPUT);
    pinMode(CTRL4, INPUT);
    pinMode(CTRL5, INPUT);
    pinMode(CTRL6, INPUT);
    pinMode(CTRL7, INPUT);
}

umdv1::~umdv1()
{
}

/*******************************************************************//**
 * The setup function sets all of the Teensy pins
 **********************************************************************/
void umdv1::setup(uint8_t param)
{
    SET_DATABUS_TO_INPUT();
    
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

    //All control signals default to input, you know, for safety
    pinMode(CTRL0, INPUT);
    pinMode(CTRL1, INPUT);
    pinMode(CTRL2, INPUT);
    pinMode(CTRL3, INPUT);
    pinMode(CTRL4, INPUT);
    pinMode(CTRL5, INPUT);
    pinMode(CTRL6, INPUT);
    pinMode(CTRL7, INPUT);

}

/*******************************************************************//**
 * The getFlashSizeFromID() function returns the flash size 
 **********************************************************************/
uint32_t umdv1::getFlashSizeFromID(uint8_t manufacturer, uint8_t device)
{
    uint32_t size = 0;
    switch( manufacturer )
    {        
        // microchip
        case 0xBF:
            switch( device )
            {
                case 0x6D: // SST39VF6401B
                case 0x6C: // SST39VF6402B
                    size = 0x800000;
                    break;
                case 0x5D: // SST39VF3201B
                case 0x5C: // SST39VF3202B
                case 0x5B: // SST39VF3201
                case 0x5A: // SST39VF3202
                    size = 0x400000;
                    break;
                case 0x4F: // SST39VF1601C
                case 0x4E: // SST39VF1602C
                case 0x4B: // SST39VF1601
                case 0x4A: // SST39VF1602
                    size = 0x200000;
                    break;
                default:
                    break;
            }
            break;
        
        // macronix
        case 0xC2:
            switch( device )
            {
                // chips which will be single per board
                // 3.3V
                case 0xC9: // MX29LV640ET
                case 0xCB: // MX29LV640EB
                    size = 0x800000;
                    break;
                case 0xA7: // MX29LV320ET
                case 0xA8: // MX29LV320EB
                    size = 0x400000;
                    break;
                case 0xC4: // MX29LV160DT
                case 0x49: // MX29LV160DB
                    size = 0x200000;
                    break;    
                // 5V
                case 0x58: // MX29F800CT
                case 0xD6: // MX29F800CB
                    size = 0x100000;
                    break;
                case 0x23: // MX29F400CT
                case 0xAB: // MX29F400CB
                    size = 0x80000;
                    break;
                case 0x51: // MX29F200CT
                case 0x57: // MX29F200CB
                    size = 0x40000;
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
    return size;
}

/*******************************************************************//**
 * The _latchAddress function latches a 24bit address to the cartridge
 * \warning contains direct port manipulation
 **********************************************************************/
void umdv1::latchAddress32(uint32_t address)
{
    uint8_t addrh,addrm,addrl;
    
    //separate address into 3 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);
    addrh = (uint8_t)(address>>16 & 0xFF);

    SET_DATABUS_TO_OUTPUT();

    //put low and mid address on bus and latch it
    DATAOUTH = addrm;
    DATAOUTL = addrl;
    
    //digitalWrite(ALE_low, HIGH);
    PORTALE |= ALE_low_setmask;
    //digitalWrite(ALE_low, LOW);
    PORTALE &= ALE_low_clrmask;

    //put high address on bus and latch it
    DATAOUTH = 0x00;
    DATAOUTL = addrh;
    
    //digitalWrite(ALE_high, HIGH);
    PORTALE |= ALE_high_setmask;
    //digitalWrite(ALE_high, LOW);
    PORTALE &= ALE_high_clrmask;
    
    //without this additional 0x00 write reads to undefined regions would
    //return the last value written to DATAOUTL
    //DATAOUTL = 0x00; // commenting this out fixed the s29gl032 problems - dunno why yet
    //DATAOUTH = 0x00;
    
    SET_DATABUS_TO_INPUT(); 
}

/*******************************************************************//**
 * The _latchAddress(uint16_t) function latches a 16bit address to the cartridge.
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umdv1::latchAddress16(uint16_t address)
{
    uint8_t addrm, addrl;
    
    //separate address into 2 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);

    SET_DATABUS_TO_OUTPUT();

    //put low and mid address on bus and latch it
    DATAOUTH = addrm;
    DATAOUTL = addrl;
    
    //digitalWrite(ALE_low, HIGH);
    PORTALE |= ALE_low_setmask;
    //digitalWrite(ALE_low, LOW);
    PORTALE &= ALE_low_clrmask;

    SET_DATABUS_TO_INPUT();
}

/*******************************************************************//**
 * The getFlashID() function stores the manufacturer, device, type (for
 * Spansion devices) and size of the flash. Since mostly all cartridges
 * are 8bits wide the flash IDs are all handled as 8bits.
 **********************************************************************/
void umdv1::getFlashID(void)
{
    
    // clear all data
    flashID.manufacturer = 0;
    flashID.device = 0;
    flashID.type = 0;
    flashID.size = 0;
          
    //mx29f800 software ID detect byte mode
    // enter software ID mode
    writeByte((uint32_t)0x0AAA, 0xAA);
    writeByte((uint32_t)0x0555, 0x55);
    writeByte((uint32_t)0x0AAA, 0x90);
    // read manufacturer
    flashID.manufacturer = readByte((uint32_t)0x0000);
    // read device
    flashID.device = readByte((uint32_t)0x0002);
    // exit software ID mode
    writeByte((uint32_t)0x0000, 0xF0);
    // figure out the size
    flashID.size = getFlashSizeFromID( flashID.manufacturer, flashID.device );

}

/*******************************************************************//**
 * The eraseChip() function erases the entire flash. If the wait parameter
 * is true the function will block with toggle bit until the erase 
 * operation has completed.
 **********************************************************************/
void umdv1::eraseChip(bool wait)
{

    //mx29f800 chip erase byte mode
    writeByte((uint32_t)0x00000AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0x80);
    writeByte16(0x0AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0x10);
	
	// if wait parameter was specified, do toggle until operation is complete
	if( wait )
	{
		// start a timer
		uint32_t intervalMillis;
        intervalMillis = millis();
        
        // wait for 4 consecutive toggle bit success reads before exiting
        while( toggleBit8(4) != 4 )
        {
            if( (millis() - intervalMillis) > 250 )
            {
                // flash to show progress
                digitalWrite(nLED, !digitalRead(nLED));
                //PC side app expects a "." before timeout
                intervalMillis = millis();
                Serial.print(".");
            }
        }
        //Send something other than a "." to indicate we are done
        Serial.print("!");
	}
}

/*******************************************************************//**
 * The toggleBit uses the toggle bit flash algorithm to determine if
 * the current program operation has completed
 **********************************************************************/
uint8_t umdv1::toggleBit8(uint8_t attempts)
{
    uint8_t retValue = 0;
    uint8_t readValue, oldValue;
    uint8_t i;
    
    //first read should always be a 1 according to datasheet
	oldValue = readByte((uint32_t)0x00000000) & 0x40;
	
	for( i=0; i<attempts; i++ )
	{
		//successive reads compare this read to the previous one for toggle bit
		readValue = readByte16(0x0000) & 0x40;
		if( oldValue == readValue )
		{
			retValue += 1;
		}else
		{
			retValue = 0;
		}
		oldValue = readValue;
	}
    
    return retValue;
}

/*******************************************************************//**
 * The toggleBit uses the toggle bit flash algorithm to determine if
 * the current program operation has completed
 **********************************************************************/
uint8_t umdv1::toggleBit16(uint8_t attempts)
{
    uint8_t retValue = 0;
    uint16_t readValue, oldValue;
    uint8_t i;
    
    //first read of bit 6 - big endian
    oldValue = readWord((uint32_t)0x00000000) & 0x4000;

    for( i=0; i<attempts; i++ ){
        //successive reads compare this read to the previous one for toggle bit
        readValue = readWord16(0x0000) & 0x4000;
        if( oldValue == readValue ){
            retValue += 1;
        }else{
            retValue = 0;
        }
        oldValue = readValue;
    }
    
    return retValue;
}

/*******************************************************************//**
 * The readByte(uint16_t) function returns a byte read from 
 * a 16bit address.
 **********************************************************************/
uint8_t umdv1::readByte16(uint16_t address)
{
    uint8_t readData;

    latchAddress16(address);
    SET_DATABUS_TO_INPUT();
    
    // read the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nRD, LOW);
    PORTCE &= nCE_clrmask;
    PORTRD &= nRD_clrmask;
    PORTRD &= nRD_clrmask;
    PORTRD &= nRD_clrmask; // wait an additional 62.5ns. ROM is slow;
    
    readData = DATAINL;
    
    //digitalWrite(nCE, HIGH);
    //digitalWrite(nRD, HIGH);
    PORTRD |= nRD_setmask;
    PORTCE |= nCE_setmask;
  
    return readData;
}

/*******************************************************************//**
 * The readByte(uint32_t) function returns a byte read from 
 * a 24bit address.
 **********************************************************************/
uint8_t umdv1::readByte(uint32_t address)
{
    uint8_t readData;

    latchAddress32(address);
    SET_DATABUS_TO_INPUT();
    
    // read the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nRD, LOW);
    PORTCE &= nCE_clrmask;
    PORTRD &= nRD_clrmask;
    PORTRD &= nRD_clrmask;
    PORTRD &= nRD_clrmask; // wait an additional 62.5ns. ROM is slow;
    
    readData = DATAINL;
    
    //digitalWrite(nCE, HIGH);
    //digitalWrite(nRD, HIGH);
    PORTRD |= nRD_setmask;
    PORTCE |= nCE_setmask;
  
    return readData;
}

/*******************************************************************//**
 * The readWord(uint16_t) function returns a word read from 
 * a 16bit address.
 * 
 * \warning converts to little endian
 **********************************************************************/
uint16_t umdv1::readWord16(uint16_t address)
{
    uint16_t readData;

    latchAddress16(address);
    SET_DATABUS_TO_INPUT();

    // read the bus
    // digitalWrite(nCE, LOW); // moving nCE after nRD completely breaks reading
    // digitalWrite(nRD, LOW);

    PORTCE &= nCE_clrmask;
    PORTRD &= nRD_clrmask;

    // removing this line breaks all reads as the fastest ROM is 70ns
    PORTRD &= nRD_clrmask; // wait an additional 62.5ns. ROM is slow, 
    
    //delayMicroseconds(1);
    //convert to little endian while reading
    readData = (uint16_t)DATAINL;
    readData <<= 8;
    readData |= (uint16_t)(DATAINH & 0x00FF);
  
    //digitalWrite(nRD, HIGH);
    //digitalWrite(nCE, HIGH);
    PORTRD |= nRD_setmask;
    PORTCE |= nCE_setmask;

    return readData;
}

/*******************************************************************//**
 * The readWord(uint32_t) function returns a word read from 
 * a 24bit address.
 * 
 * \warning converts to little endian
 **********************************************************************/
uint16_t umdv1::readWord(uint32_t address)
{
    uint16_t readData;

    latchAddress32(address);
    SET_DATABUS_TO_INPUT();

    // read the bus
    // digitalWrite(nCE, LOW); // moving nCE after nRD completely breaks reading
    // digitalWrite(nRD, LOW);

    PORTCE &= nCE_clrmask;
    PORTRD &= nRD_clrmask;

    // removing this line breaks all reads as the fastest ROM is 70ns
    PORTRD &= nRD_clrmask; // wait an additional 62.5ns. ROM is slow, 
    
    //delayMicroseconds(1);
    //convert to little endian while reading
    readData = (uint16_t)DATAINL;
    readData <<= 8;
    readData |= (uint16_t)(DATAINH & 0x00FF);
  
    //digitalWrite(nRD, HIGH);
    //digitalWrite(nCE, HIGH);
    PORTRD |= nRD_setmask;
    PORTCE |= nCE_setmask;

    return readData;
}

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 16bit
 * address.
 * 
 * \warning word is converted to big endian
 **********************************************************************/
void umdv1::writeWord16(uint16_t address, uint16_t data)
{
    latchAddress16(address);
    SET_DATABUS_TO_OUTPUT();

    //put word on bus
    DATAOUTH = (uint8_t)(data);
    DATAOUTL = (uint8_t)(data>>8);

    // write to the bus, ensure RD is high because it controls the level shifters direction
    // digitalWrite(nCE, LOW);
    // digitalWrite(nWR, LOW);

    // delayMicroseconds(1);
    PORTCE &= nCE_clrmask;
    // PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask; // waste 62.5ns - nWR should be low for 125ns
    
    // digitalWrite(nWR, HIGH);
    // digitalWrite(nCE, HIGH);
    PORTWR |= nWR_setmask;
    PORTCE |= nCE_setmask;

    SET_DATABUS_TO_INPUT();
}

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 24bit
 * address.
 * 
 * \warning word is converted to big endian
 **********************************************************************/
void umdv1::writeWord(uint32_t address, uint16_t data)
{
    latchAddress32(address);
    SET_DATABUS_TO_OUTPUT();

    //put word on bus
    DATAOUTH = (uint8_t)(data);
    DATAOUTL = (uint8_t)(data>>8);

    // write to the bus, ensure RD is high because it controls the level shifters direction
    // digitalWrite(nCE, LOW);
    // digitalWrite(nWR, LOW);

    // delayMicroseconds(1);
    PORTCE &= nCE_clrmask;
    // PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask; // waste 62.5ns - nWR should be low for 125ns
    
    // digitalWrite(nWR, HIGH);
    // digitalWrite(nCE, HIGH);
    PORTWR |= nWR_setmask;
    PORTCE |= nCE_setmask;

    SET_DATABUS_TO_INPUT();
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 16bit
 * address. The upper 8 address bits (23..16) are not modified
 * by this function so this can be used to perform quicker successive
 * writes within a 64k boundary.
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umdv1::writeByte16(uint16_t address, uint8_t data)
{

    latchAddress16(address);
    SET_DATABUS_TO_OUTPUT();
    DATAOUTL = data;
    
    // write to the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nWR, LOW);
    PORTCE &= nCE_clrmask;
    PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask; // waste 62.5ns - nWR should be low for 125ns
    
    //digitalWrite(nWR, HIGH);
    //digitalWrite(nCE, HIGH);
    PORTWR |= nWR_setmask;
    PORTCE |= nCE_setmask;
    
    SET_DATABUS_TO_INPUT();
    
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 24bit
 * address.
 **********************************************************************/
void umdv1::writeByte(uint32_t address, uint8_t data)
{

    latchAddress32(address);
    SET_DATABUS_TO_OUTPUT();
    DATAOUTL = data;
    
    // write to the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nWR, LOW);
    PORTCE &= nCE_clrmask;
    PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask;
    PORTWR &= nWR_clrmask; // waste 62.5ns - nWR should be low for 125ns
    
    //digitalWrite(nWR, HIGH);
    //digitalWrite(nCE, HIGH);
    PORTWR |= nWR_setmask;
    PORTCE |= nCE_setmask;
    
    SET_DATABUS_TO_INPUT();
    
}

/*******************************************************************//**
 * The programByte function programs a byte into the flash array at a
 * 24bit address. If the wait parameter is true the function will wait 
 * for the program operation to complete using data polling before 
 * returning.
 * 
 * \warning Sector or entire IC must be erased prior to programming
 **********************************************************************/
void umdv1::programByte(uint32_t address, uint8_t data, bool wait)
{
	
    //mx29f800 program byte mode
    writeByte((uint32_t)0x00000AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0xA0);
	
	//write the data
	writeByte(address, data);
	
	//use data polling to validate end of program cycle
	if(wait)
	{
		while( toggleBit8(2) != 2 );
	}
	
}

/*******************************************************************//**
 * The programByte function programs a word into the flash array at a
 * 24bit address. If the wait parameter is true the function will wait 
 * for the program operation to complete using data polling before 
 * returning.
 * 
 * \warning Sector or entire IC must be erased prior to programming
 **********************************************************************/
void umdv1::programWord(uint32_t address, uint16_t data, bool wait)
{
    writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
	writeWord16((0x02AA << 1), 0x5500);
    writeWord16((0x0555 << 1), 0xA000);
    
    //write the data
    writeWord( (uint32_t)address, data );
    //delayMicroseconds(10);

    //use data polling to validate end of program cycle
	if(wait){
		while( toggleBit16(2) != 2 );
	}
}

void umdv1::writeByteTime(uint32_t address, uint8_t data)
{
    // do nothing
}


/*******************************************************************//**
 * The reverseByte function uses a table to reverse the bits in a byte
 * Useful for reading reverse databus on PC Engine
 **********************************************************************/
uint8_t umdv1::mirror_byte(uint8_t data)
{
    static const uint8_t table[] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };
    return table[data];
}