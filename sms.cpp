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
void sms::setup()
{

    pinMode(nWR, OUTPUT);
    digitalWrite(nWR, HIGH);
    pinMode(nRD, OUTPUT);
    digitalWrite(nRD, HIGH);
    pinMode(nCE, OUTPUT);
    digitalWrite(nRD, HIGH);

    //set default slot registers
    setSMSSlotRegister(0,SMS_SLOT_0_ADDR);
    setSMSSlotRegister(1,SMS_SLOT_1_ADDR);
    setSMSSlotRegister(2,SMS_SLOT_2_ADDR);

    SMS_SelectedPage = 2;

    info.cartType = SMS;
    info.busSize = 8;
}

/*******************************************************************//**
 * The getFlashID() function stores the manufacturer, device, type (for
 * Spansion devices) and size of the flash. SMS flash ID reads need to
 * ensure the proper mapper register values are set.
 **********************************************************************/
void sms::getFlashID(uint8_t alg)
{
    
    // clear all data
    flashID.manufacturer = 0;
    flashID.device = 0;
    flashID.type = 0;
    flashID.size = 0;
    flashID.alg = alg;
    
    //set default slot registers
    setSMSSlotRegister(0,SMS_SLOT_0_ADDR);
    setSMSSlotRegister(1,SMS_SLOT_1_ADDR);
    setSMSSlotRegister(2,SMS_SLOT_2_ADDR);
    
    //enable rom write enable bit
    writeByte((uint16_t)SMS_CONF_REG_ADDR,0x80);
    
    if( alg == 0 )
    {        
        //mx29f800 software ID detect byte mode
        // enter software ID mode
        writeByte((uint16_t)0x0AAA, 0xAA);
        writeByte((uint16_t)0x0555, 0x55);
        writeByte((uint16_t)0x0AAA, 0x90);
        // read manufacturer
        flashID.manufacturer = readByte((uint32_t)0x0000);
        // read device
        flashID.device = readByte((uint32_t)0x0001);
        // exit software ID mode
        writeByte((uint16_t)0x0000, 0xF0);
        // figure out the size
        flashID.size = getFlashSizeFromID( flashID.manufacturer, flashID.device, 0 );
    }else
    {
        //SST39SF0x0 software ID detect
        //get first byte
        writeByte((uint16_t)0x5555,0xAA);
        writeByte((uint16_t)0x2AAA,0x55);
        writeByte((uint16_t)0x5555,0x90);
        // read manufacturer
        flashID.manufacturer = readByte((uint16_t)0x0000);
        // read device
        flashID.device = readByte((uint16_t)0x0001);
        // exit software ID mode
        writeByte((uint16_t)0x0000, 0xF0);
        // figure out the size
        flashID.size = getFlashSizeFromID( flashID.manufacturer, flashID.device, 0 );
    }
    
    // disable rom write enable bit
    writeByte((uint16_t)SMS_CONF_REG_ADDR,0x00);
}

/*******************************************************************//**
 * The calcChecksum() function calculates the word sum of all words
 * in the SMS cartridge
 **********************************************************************/
void sms::calcChecksum()
{
    
    uint8_t romSizeCode;
    uint32_t romSize, skipChecksumStart, skipChecksumEnd;
    
    //# sizecode : (romSize, skipChecksumStart, skipChecksumEnd)
    //romSizeData = {
        //10 : (8192,    0x1FEF, 0x2000),
        //11 : (16384,   0x3FEF, 0x4000),
        //12 : (32768,   0x7FEF, 0x8000),
        //13 : (49152,   0xBFEF, 0xC000),
        //14 : (65536,   0x7FEF, 0x8000),
        //15 : (131072,  0x7FEF, 0x8000),
        //0  : (262144,  0x7FEF, 0x8000),
        //1  : (525288,  0x7FEF, 0x8000),
        //2  : (1048576, 0x7FEF, 0x8000), 
    //}
    
    //get size code in 0x7FFF : TODO - fancy search for 'SEGA' string
    romSizeCode = readByte( (uint16_t)0x7FFF ) & 0x0F;
    
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
    
}

/*******************************************************************//**
 * The readByte(uint32_t) function returns a byte read from 
 * a 24bit address using mapper
 **********************************************************************/
uint8_t sms::readByte(uint32_t address)
{

    uint8_t readData;

    //no banking necessary for addresses up to 0x7FFF
    if( address < SMS_SLOT_2_ADDR )
    {
        latchAddress((uint16_t)address);

    }else
    {
        latchAddress(setSMSSlotRegister(2, address));
    }

    SET_DATABUS_TO_INPUT();
    
    // read the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nRD, LOW);
    PORTCE &= nCE_clrmask;
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
 * The setSMSSlotRegister function updates the cartridge slot register
 * with the correct bank number of the corresponding address
 **********************************************************************/
uint16_t sms::setSMSSlotRegister(uint8_t slotNum, uint32_t address)
{
    uint8_t selectedPage;
    uint16_t virtualAddress;
    
    selectedPage = (uint8_t)(address >> 14);
    switch( slotNum )
    {
        case 0:
            if(SMS_SelectedPage != selectedPage )
            {
                writeByte( (uint16_t)SMS_SLOT_0_REG_ADDR, selectedPage );
            }
            virtualAddress = ( SMS_SLOT_0_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
        case 1:
            if(SMS_SelectedPage != selectedPage )
            {
                writeByte( (uint16_t)SMS_SLOT_1_REG_ADDR, selectedPage );
            }
            virtualAddress = ( SMS_SLOT_1_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
        case 2:
            if(SMS_SelectedPage != selectedPage )
            {
                writeByte( (uint16_t)SMS_SLOT_2_REG_ADDR, selectedPage );
            }
            virtualAddress = ( SMS_SLOT_2_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
        default:
            if(SMS_SelectedPage != selectedPage )
            {
                writeByte( (uint16_t)SMS_SLOT_2_REG_ADDR, selectedPage );
            }
            virtualAddress = ( SMS_SLOT_2_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
    }
    
    //kind of assumes we're only mapping in 1 slot (#2)
    SMS_SelectedPage = selectedPage;

    return virtualAddress;
}

/*******************************************************************//**
 * The enableSram() function writes to the time register to enable
 * the SRAM latch
 **********************************************************************/
void sms::enableSram(uint8_t param)
{

}

/*******************************************************************//**
 * The enableSram() function writes to the time register to disable
 * the SRAM latch
 **********************************************************************/
void sms::disableSram(uint8_t param)
{

}

