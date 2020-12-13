/*******************************************************************//**
 *  \file genesis.cpp
 *  \author René Richard
 *  \brief This program contains specific functions for the sms cartridge
 *
 * \copyright This file is part of Universal Mega Dumper.
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
#include "umdbase.h"
#include "sms.h"

/*******************************************************************//**
 * The constructor
 **********************************************************************/
sms::sms() 
{
}

/*******************************************************************//**
 * Setup the ports for Genesis mode
 **********************************************************************/
void sms::setup(uint8_t alg)
{

    pinMode(nWR, OUTPUT);
    digitalWrite(nWR, HIGH);
    pinMode(nRD, OUTPUT);
    digitalWrite(nRD, HIGH);
    pinMode(nCE, OUTPUT);
    digitalWrite(nRD, HIGH);

    // reset pulse
    pinMode(SMS_nRST, OUTPUT);
    digitalWrite(SMS_nRST, LOW);
    delay(1);
    digitalWrite(SMS_nRST, HIGH);

    //set default slot registers
    setSMSSlotRegister(1, (uint32_t)SMS_SLOT_1_ADDR);
    setSMSSlotRegister(2, (uint32_t)SMS_SLOT_2_ADDR);

    SMS_SelectedPage = 2;
    info.console = SMS;
    info.busSize = 8;
}

/*******************************************************************//**
 * The getFlashID() function stores the manufacturer, device, type (for
 * Spansion devices) and size of the flash. SMS flash ID reads need to
 * ensure the proper mapper register values are set.
 **********************************************************************/
void sms::getFlashID(void)
{
    // clear all data
    flashID.manufacturer = 0;
    flashID.device = 0;
    flashID.type = 0;
    flashID.size = 0;
    
    //set default slot registers
    setSMSSlotRegister(1, (uint32_t)SMS_SLOT_1_ADDR);
    setSMSSlotRegister(2, (uint32_t)SMS_SLOT_2_ADDR);
    
    //enable rom write enable bit
    romWrites(true);
          
    //mx29f800 software ID detect byte mode
    // enter software ID mode
    writeByte16(0x0AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0x90);
    // read manufacturer
    flashID.manufacturer = readByte16(0x0000);
    // read device
    flashID.device = readByte16(0x0001);
    // exit software ID mode
    writeByte16(0x0000, 0xF0);
    // figure out the size
    flashID.size = getFlashSizeFromID(flashID.manufacturer, flashID.device, 0);
    
    // disable rom write enable bit
    romWrites(false);
}

/*******************************************************************//**
 * The calcChecksum() function calculates the word sum of all words
 * in the SMS cartridge
 **********************************************************************/
void sms::calcChecksum()
{
    uint32_t address;
    uint16_t timeOut = 0;
    
    checksum.expected = (uint16_t)readByte((uint32_t)0x00007FFB);
    checksum.expected <<= 8;
    checksum.expected |= (uint16_t)readByte((uint32_t)0x00007FFA);
    checksum.romSize = getRomSize();
    
    checksum.calculated = 0;
    address = 0;
    
    while( address <= skipChecksumStart )
    {
        checksum.calculated += (uint16_t)readByte(address++);
        
        //PC side app expects a "." before timeout
        if( timeOut++ > 0x3FFF )
        {
            timeOut = 0;
            Serial.print(".");
        }
    }
    
    //jump to end of header
    
    address = skipChecksumEnd;
    while( address < checksum.romSize )
    {
        checksum.calculated += (uint16_t)readByte(address++);
        
        //PC side app expects a "." before timeout
        if( timeOut++ > 0x3FFF )
        {
            timeOut = 0;
            Serial.print(".");
        }
    }
    
    //Send something other than a "." to indicate we are done
    Serial.print("!");

}

/*******************************************************************//**
 * The getRomSize() function retrieves the romSize parameter form the
 * ROM's header
 **********************************************************************/
uint32_t sms::getRomSize()
{
    uint8_t romSizeCode;
    uint32_t romSize;
    
    //get size code in 0x7FFF : TODO - fancy search for 'TMR SEGA' string
    romSizeCode = readByte((uint32_t)0x00007FFF) & 0x0F;
    
    switch(romSizeCode)
    {
        case 10:
            romSize = 8192;
            skipChecksumStart = 0x1FEF;
            skipChecksumEnd = 0x2000;
            break;
        case 11:
            romSize = 16384;
            skipChecksumStart = 0x3FEF;
            skipChecksumEnd = 0x4000;
            break;
        case 12:
            romSize = 32768;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
        case 13:
            romSize = 49152;
            skipChecksumStart = 0xBFEF;
            skipChecksumEnd = 0xC000;
            break;
        case 14:
            romSize = 65536;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
        case 15:
            romSize = 131072;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
        case 0:
            romSize = 262144;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
        case 1:
            romSize = 525288;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
        case 2:
            romSize = 1048576;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
        default:
            romSize = 262144;
            skipChecksumStart = 0x7FEF;
            skipChecksumEnd = 0x8000;
            break;
    }
    
    return romSize;
}

/*******************************************************************//**
 * The readByte(uint32_t) function returns a byte read from 
 * a 24bit address using mapper
 **********************************************************************/
uint8_t sms::readByte(uint32_t address)
{

    uint8_t readData;

    if(address < (uint32_t)SMS_SLOT_2_ADDR ){
        latchAddress16((uint16_t)address);
    }else{
        //latch the address and set slot 2
        setSMSSlotRegister(2, address);
        latchAddress16(SMS_SLOT_2_ADDR | ((uint16_t)address & SMS_SLOT_MASK));
    }
    
    SET_DATABUS_TO_INPUT();
    
    // read the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nRD, LOW);
    
    readData = DATAINL;
    
    digitalWrite(nCE, HIGH);
    digitalWrite(nRD, HIGH);
    
    return readData;
    
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 24bit
 * address.
 **********************************************************************/
void sms::writeByte(uint32_t address, uint8_t data)
{

    if(address < (uint32_t)SMS_SLOT_2_ADDR ){
        latchAddress16((uint16_t)address);
    }else{
        //latch the address and set slot 2
        latchAddress16(setSMSSlotRegister(2, address));
    }

    SET_DATABUS_TO_OUTPUT();
    DATAOUTL = data;
    
    // write to the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nWR, LOW);
    // PORTCE &= nCE_clrmask;
    // PORTWR &= nWR_clrmask;
    
    // PORTWR &= nWR_clrmask; // waste 62.5ns - nWR should be low for 125ns
    
    digitalWrite(nWR, HIGH);
    digitalWrite(nCE, HIGH);
    // PORTWR |= nWR_setmask;
    // PORTCE |= nCE_setmask;
    
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
void sms::programByte(uint32_t address, uint8_t data, bool wait)
{

    //enable rom write enable bit
    romWrites(true);

    //mx29f800 program byte mode
    writeByte16(0x0AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0xA0);
	sms::writeByte(address, data);
	
    //disable rom write enable bit
    romWrites(false);

	//use data polling to validate end of program cycle
	if(wait)
	{
		while( toggleBit8(4) != 4 );
	}
}

/*******************************************************************//**
 * The eraseChip() function erases the entire flash. If the wait parameter
 * is true the function will block with toggle bit until the erase 
 * operation has completed.
 **********************************************************************/
void sms::eraseChip(bool wait)
{
    //enable rom write enable bit
    romWrites(true);

    //mx29f800 chip erase byte mode
    writeByte16(0x0AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0x80);
    writeByte16(0x0AAA, 0xAA);
    writeByte16(0x0555, 0x55);
    writeByte16(0x0AAA, 0x10);
	
    //disable rom write enable bit
    romWrites(false);

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
 * The setSMSSlotRegister function updates the cartridge slot register
 * with the correct bank number of the corresponding address and 
 * returns the virtual address to read from in the selected slot
 **********************************************************************/
uint16_t sms::setSMSSlotRegister(uint8_t slotNum, uint32_t address)
{
    uint8_t selectedPage;
    uint16_t virtualAddress;
    
    selectedPage = (uint8_t)(address>>14);
    //Serial.println("");
    //Serial.print(F("page="));
    //Serial.println(selectedPage, DEC);
    switch(slotNum)
    {
        case 1:
            umdbase::writeByte16(SMS_SLOT_1_REG_ADDR, selectedPage);
            virtualAddress = (SMS_SLOT_1_ADDR | ( (uint16_t)address & 0x3FFF));
            break;
        case 2:
            umdbase::writeByte16(SMS_SLOT_2_REG_ADDR, selectedPage);
            virtualAddress = (SMS_SLOT_2_ADDR | ( (uint16_t)address & 0x3FFF));
            break;
        default:
            umdbase::writeByte16(SMS_SLOT_2_REG_ADDR, selectedPage);
            virtualAddress = (SMS_SLOT_2_ADDR | ( (uint16_t)address & 0x3FFF));
            break;
    }
    
    //kind of assumes we're only mapping in 1 slot (#2)
    SMS_SelectedPage = selectedPage;
    //Serial.println("");
    //Serial.print(F("vadr="));
    //Serial.println(virtualAddress, HEX);
    return virtualAddress;
}

/*******************************************************************//**
 * The enableSram() function writes to the conf register to enable
 * the SRAM latch
 **********************************************************************/
void sms::enableSram(uint8_t param)
{
    writeByte16(SMS_CONF_REG_ADDR, 0x88);
}

/*******************************************************************//**
 * The enableSram() function writes to the conf register to disable
 * the SRAM latch
 **********************************************************************/
void sms::disableSram(uint8_t param)
{
    writeByte16(SMS_CONF_REG_ADDR, 0x00);
}

/*******************************************************************//**

 **********************************************************************/
void sms::romWrites(bool enable)
{
    if(enable){
        writeByte16(SMS_CONF_REG_ADDR, 0x80);
    }else{
        writeByte16(SMS_CONF_REG_ADDR, 0x00);
    }
    
}

