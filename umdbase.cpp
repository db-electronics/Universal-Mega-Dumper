/*******************************************************************//**
 *  \file umdbase.cpp
 *  \author René Richard
 *  \brief This program allows to read and write to various game cartridges 
 *         including: Genesis, Coleco, SMS, PCE - with possibility for 
 *         future expansion.
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

/*******************************************************************//**
 * The constructor
 **********************************************************************/
umdbase::umdbase() 
{
    _setDatabusInput();

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
 * The _setDatabusInput function sets the databus to inputs and 
 * deactivates the built-in pullup resistors
 **********************************************************************/
inline void umdbase::_setDatabusInput()
{
    //set data to inputs
    DATAH_DDR = 0x00;
    DATAL_DDR = 0x00;
    DATAOUTH = 0x00;
    DATAOUTL = 0x00;
}

/*******************************************************************//**
 * The _setDatabusOutput function sets the databus to outputs
 **********************************************************************/
inline void umdbase::_setDatabusOutput()
{
    //set data to outputs
    DATAH_DDR = 0xFF;
    DATAL_DDR = 0xFF;
}

/*******************************************************************//**
 * The resetCart() function issues a 100ms active-low reset pulse to
 * the pin specified by the variable _resetPin.
 **********************************************************************/
virtual void umdbase::resetCart()
{
    digitalWrite(_resetPin, LOW);
    delay(100);
    digitalWrite(_resetPin, HIGH);
}

/*******************************************************************//**
 * The getFlashID() function stores the manufacturer, device, type (for
 * Spansion devices) and size of the flash. Since mostly all cartridges
 * are 8bits wide the flash IDs are all handled as 8bits.
 **********************************************************************/
virtual void umdbase::getFlashID(uint8_t alg)
{
    
    // clear all data
    flashID.manufacturer = 0;
    flashID.device = 0;
    flashID.type = 0;
    flashID.size = 0;
    
    if( alg == 0 )
    {        
        //mx29f800 software ID detect byte mode
        // enter software ID mode
        writeByte((uint32_t)0x0AAA, 0xAA);
        writeByte((uint16_t)0x0555, 0x55);
        writeByte((uint16_t)0x0AAA, 0x90);
        // read manufacturer
        flashID.manufacturer = readByte((uint32_t)0x0000, false);
        // read device
        flashID.device = readByte((uint32_t)0x0001, false);
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
        flashID.manufacturer = readByte((uint16_t)0x0000, false);
        // read device
        flashID.device = readByte((uint16_t)0x0001, false);
        // exit software ID mode
        writeByte((uint16_t)0x0000, 0xF0);
        // figure out the size
        flashID.size = getFlashSizeFromID( flashID.manufacturer, flashID.device, 0 );
    }
}

/*******************************************************************//**
 * The _latchAddress function latches a 24bit address to the cartridge
 * \warning contains direct port manipulation
 **********************************************************************/
inline void umdbase::_latchAddress(uint32_t address)
{
    uint8_t addrh,addrm,addrl;
    
    //separate address into 3 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);
    addrh = (uint8_t)(address>>16 & 0xFF);

    _setDatabusOutput();

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
    
    //digitalWrite(ALE_low, HIGH);
    PORTALE |= ALE_high_setmask;
    //digitalWrite(ALE_low, LOW);
    PORTALE &= ALE_high_clrmask;
    
    //without this additional 0x00 write reads to undefined regions would
    //return the last value written to DATAOUTL
    DATAOUTL = 0x00; 
    _setDatabusInput(); 

}

/*******************************************************************//**
 * The _latchAddress(uint16_t) function latches a 16bit address to the cartridge.
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
inline void umdbase::_latchAddress(uint16_t address)
{
    uint8_t addrm,addrl;
    
    //separate address into 2 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);

    _setDatabusOutput();

    //put low and mid address on bus and latch it
    DATAOUTH = addrm;
    DATAOUTL = addrl;
    
    //digitalWrite(ALE_low, HIGH);
    PORTALE |= ALE_low_setmask;
    //digitalWrite(ALE_low, LOW);
    PORTALE &= ALE_low_clrmask;
    
    _setDatabusInput();
}

/*******************************************************************//**
 * The readWord(uint32_t) function returns a word read from 
 * a 24bit address.
 * 
 * \warning converts to little endian
 **********************************************************************/
virtual uint16_t umdbase::readWord(uint32_t address)
{
    uint16_t readData;

    _latchAddress(address);
    _setDatabusInput();

    // read the bus
    //digitalWrite(nCE, LOW);
    //digitalWrite(nRD, LOW);
    PORTCE &= nCE_clrmask;
    PORTRD &= nRD_clrmask;
    
    PORTRD &= nRD_clrmask; // wait an additional 62.5ns. ROM is slow
    
    //convert to little endian while reading
    readData = (uint16_t)DATAINL;
    readData <<= 8;
    readData |= (uint16_t)(DATAINH & 0x00FF);
  
    //digitalWrite(nCE, HIGH);
    //digitalWrite(nRD, HIGH);
    PORTRD |= nRD_setmask;
    PORTCE |= nCE_setmask;

    return readData;
}

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 24bit
 * address.
 * 
 * \warning word is converted to big endian
 **********************************************************************/
void umdbase::writeWord(uint32_t address, uint16_t data)
{
    _latchAddress(address);
    _setDatabusOutput();

    //put word on bus
    DATAOUTH = (uint8_t)(data);
    DATAOUTL = (uint8_t)(data>>8);

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

    _setDatabusInput();
}
