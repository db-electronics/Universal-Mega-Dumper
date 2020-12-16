/*******************************************************************//**
 *  \file genesis.cpp
 *  \author René Richard
 *  \brief This program contains specific functions for the genesis cartridge
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

#include <Arduino.h>
#include "genesis.h"

/*******************************************************************//**
 * The constructor
 **********************************************************************/
genesis::genesis() 
{
}

/*******************************************************************//**
 * Setup the ports for Genesis mode
 **********************************************************************/
void genesis::setup(uint8_t param)
{
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

    info.console = GENESIS;
    info.mirrored_bus = false;
    info.bus_size = 16;
    
    _resetPin = GEN_nVRES;
    //resetCart();   
}

/*******************************************************************//**
 * The getFlashID() function stores the manufacturer, device, type (for
 * Spansion devices) and size of the flash. Genesis needs to do this 
 * in word mode therefore we override the virtual base function.
 **********************************************************************/
void genesis::getFlashID(void)
{
    uint16_t readData = 0;
    
    // clear all data
    flashID.manufacturer = 0;
    flashID.device = 0;
    flashID.type = 0;
    flashID.size = 0;
    flashID.buffermode = 0;

    // enter software ID mode
    writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
    writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
    writeWord( (uint32_t)(0x000555 << 1), 0x9000);
    // read manufacturer
    readData = readWord( (uint32_t)(0x000000) );
    flashID.manufacturer = (uint8_t)(readData >> 8);
    // read device
    readData = readWord( (uint32_t)(0x000001 << 1) );
    flashID.device = (uint8_t)(readData >> 8);
    // spansion devices have additional data here
    readData  = readWord( (uint32_t)(0x00000E << 1) );
    flashID.type = (uint8_t)(readData >> 8);
    // exit software ID mode
    writeWord( (uint32_t)0x000000, 0xF000);
    // figure out the size
    flashID.size = getFlashSizeFromID( flashID.manufacturer, flashID.device, flashID.type );

    if(flashID.manufacturer == 0x01){
        flashID.buffermode = 1;
    }
}

/*******************************************************************//**
 * The calcChecksum() function calculates the word sum of all words
 * in the Genesis cartridge
 **********************************************************************/
void genesis::calcChecksum()
{
    uint32_t address;
    uint16_t timeOut = 0;
    
    checksum.expected = readBigWord( 0x00018E );
    checksum.romSize = getRomSize();
    
    checksum.calculated = 0;
    address = 0x200;
    
    while( address < checksum.romSize )
    {
        checksum.calculated += readBigWord(address);
        address += 2;
        
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
 * The programWordBuffer() function uses the S29GL0xx buffer mode
 * to program a block of words at once
 **********************************************************************/
void genesis::programWordBuffer(uint32_t address, uint16_t * buf, uint8_t size)
{
    uint8_t i;
    uint32_t sectorAddr, writeAddr;

    // address must full within a 32b/16w boundary
    sectorAddr = 0xFFFFFFE0 & address;
    writeAddr = sectorAddr;

    // enter write to buffer mode
    writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
    writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
    writeWord( sectorAddr, 0x2500);
    writeWord( sectorAddr, size-1);

    // write contents to program flash buffer
    for(i=0; i<size; i++){
        writeWord( writeAddr, *(buf++) );
        writeAddr += 2;
    }

    // program buffer to flash
    writeWord( sectorAddr, 0x2900);

    //use data polling to validate end of program cycle
    while( toggleBit16(4) != 4 );
}

/*******************************************************************//**
 * The getRomSize() function retrieves the romSize parameter form the
 * ROM's header
 **********************************************************************/
uint32_t genesis::getRomSize()
{
    uint32_t romSize;
    
    romSize = readBigWord( 0x0001A4 );
    romSize <<= 16;
    romSize |= readBigWord( 0x0001A6 );
    romSize += 1;
    
    return romSize;
}

/*******************************************************************//**
 * The eraseChip() function erases the entire flash. If the wait parameter
 * is true the function will block with toggle bit until the erase 
 * operation has completed.
 **********************************************************************/
void genesis::eraseChip(bool wait)
{
	writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
    writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
    writeWord( (uint32_t)(0x000555 << 1), 0x8000);
    writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
    writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
    writeWord( (uint32_t)(0x000555 << 1), 0x1000);
	
	// if wait parameter was specified, do toggle until operation is complete
	if( wait )
	{
		// start a timer
		uint32_t intervalMillis;
        intervalMillis = millis();
        
        // wait for 4 consecutive toggle bit success reads before exiting
        while( toggleBit16(4) != 4 )
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
 * The writeByte function strobes a byte into the cartridge at a 24bit
 * address onto the odd byte.
 **********************************************************************/
void genesis::writeByte(uint32_t address, uint8_t data)
{

    latchAddress32(address);
    SET_DATABUS_TO_OUTPUT();
    DATAOUTH = data;
    
    // write to the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nWR, LOW);
    PORTCE &= nCE_clrmask;
    PORTWR &= nWR_clrmask;
    
    PORTWR &= nWR_clrmask; // waste 62.5ns - nWR should be low for 125ns
    
    //digitalWrite(nWR, HIGH);
    //digitalWrite(nCE, HIGH);
    PORTWR |= nWR_setmask;
    PORTCE |= nCE_setmask;
    
    SET_DATABUS_TO_INPUT();
    
}

/*******************************************************************//**
 * The writeByteTime function strobes a byte into nTIME region
 * while enabling the rest of the regular signals
 **********************************************************************/
void genesis::writeByteTime(uint32_t address, uint8_t data)
{
    
    latchAddress32(address);
    SET_DATABUS_TO_OUTPUT();

    //put byte on bus
    DATAOUTL = data;
    
    // write to the bus
    digitalWrite(GEN_nLWR, LOW);
    digitalWrite(GEN_nTIME, LOW);
    digitalWrite(GEN_nTIME, HIGH);
    digitalWrite(GEN_nLWR, HIGH);
 
    SET_DATABUS_TO_INPUT();
}

/*******************************************************************//**
 * The readWord(uint32_t) function returns a big endian word read from 
 * a 24bit address.
 **********************************************************************/
uint16_t genesis::readBigWord(uint32_t address)
{
    uint16_t readData;

    latchAddress32(address);
    SET_DATABUS_TO_INPUT();

    // read the bus, setup the direction on the level shifters first to avoid contention
    digitalWrite(nRD, LOW);
    digitalWrite(nCE, LOW);
    // PORTCE &= nCE_clrmask;
    // PORTRD &= nRD_clrmask;
    // PORTRD &= nRD_clrmask; // wait an additional 62.5ns. ROM is slow
    
    //convert to big endian while reading
    readData = (uint16_t)DATAINH;
    readData <<= 8;
    readData |= (uint16_t)(DATAINL & 0x00FF);
  
    digitalWrite(nCE, HIGH);
    digitalWrite(nRD, HIGH);
    // PORTRD |= nRD_setmask;
    // PORTCE |= nCE_setmask;

    return readData;
}

/*******************************************************************//**
 * The enableSram() function writes to the time register to enable
 * the SRAM latch
 **********************************************************************/
void genesis::enableSram(uint8_t param)
{
    writeByteTime(0,3);
}

/*******************************************************************//**
 * The enableSram() function writes to the time register to disable
 * the SRAM latch
 **********************************************************************/
void genesis::disableSram(uint8_t param)
{
    writeByteTime((uint32_t)0,0);
}
