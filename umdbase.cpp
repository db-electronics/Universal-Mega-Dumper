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
    setDatabusInput();

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
 * The setup function sets all of the Teensy pins
 **********************************************************************/
void umdbase::setup()
{
    setDatabusInput();
    
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
void umdbase::setDatabusInput()
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
void umdbase::setDatabusOutput()
{
    //set data to outputs
    DATAH_DDR = 0xFF;
    DATAL_DDR = 0xFF;
}

/*******************************************************************//**
 * The getFlashID() function stores the manufacturer, device, type (for
 * Spansion devices) and size of the flash. Since mostly all cartridges
 * are 8bits wide the flash IDs are all handled as 8bits.
 **********************************************************************/
void umdbase::getFlashID(uint8_t alg)
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
}

/*******************************************************************//**
 * The _latchAddress function latches a 24bit address to the cartridge
 * \warning contains direct port manipulation
 **********************************************************************/
void umdbase::latchAddress(uint32_t address)
{
    uint8_t addrh,addrm,addrl;
    
    //separate address into 3 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);
    addrh = (uint8_t)(address>>16 & 0xFF);

    setDatabusOutput();

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
    
    //setDatabusInput(); 
	set_databus_inputs();
}

/*******************************************************************//**
 * The _latchAddress(uint16_t) function latches a 16bit address to the cartridge.
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umdbase::latchAddress(uint16_t address)
{
    uint8_t addrm,addrl;
    
    //separate address into 2 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);

    setDatabusOutput();

    //put low and mid address on bus and latch it
    DATAOUTH = addrm;
    DATAOUTL = addrl;
    
    //digitalWrite(ALE_low, HIGH);
    PORTALE |= ALE_low_setmask;
    //digitalWrite(ALE_low, LOW);
    PORTALE &= ALE_low_clrmask;
    
    //setDatabusInput();
    set_databus_inputs();
}

/*******************************************************************//**
 * The readByte(uint16_t) function returns a byte read from 
 * a 16bit address.
 **********************************************************************/
uint8_t umdbase::readByte(uint16_t address)
{
    uint8_t readData;

    latchAddress(address);
    setDatabusInput();
    
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
 * The readByte(uint32_t) function returns a byte read from 
 * a 24bit address.
 **********************************************************************/
uint8_t umdbase::readByte(uint32_t address)
{
    uint8_t readData;

    latchAddress(address);
    setDatabusInput();
    
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
 * The readWord(uint32_t) function returns a word read from 
 * a 24bit address.
 * 
 * \warning converts to little endian
 **********************************************************************/
uint16_t umdbase::readWord(uint32_t address)
{
    uint16_t readData;

    latchAddress(address);
    //setDatabusInput();
    set_databus_inputs();

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
    latchAddress(address);
    setDatabusOutput();

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

    setDatabusInput();
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 16bit
 * address. The upper 8 address bits (23..16) are not modified
 * by this function so this can be used to perform quicker successive
 * writes within a 64k boundary.
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umdbase::writeByte(uint16_t address, uint8_t data)
{

    latchAddress(address);
    setDatabusOutput();
    DATAOUTL = data;
    
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
    
    setDatabusInput();
    
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 24bit
 * address.
 **********************************************************************/
void umdbase::writeByte(uint32_t address, uint8_t data)
{

    latchAddress(address);
    setDatabusOutput();
    DATAOUTL = data;
    
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
    
    setDatabusInput();
    
}

/*******************************************************************//**
 * The getFlashSizeFromID() function returns the flash size 
 **********************************************************************/
uint32_t umdbase::getFlashSizeFromID(uint8_t manufacturer, uint8_t device, uint8_t type)
{
    uint32_t size = 0;
    switch( manufacturer )
    {
        // spansion
        case 0x01:
            switch( type )
            {
                case 0x10: // SG29GL064N
                case 0x0C: // SG29GL064N
                    size = 0x800000;
                    break;
                case 0x1A: // SG29GL032N
                case 0x1D: // SG29GL032N
                    size = 0x400000;
                    break;
                default:
                    break;
            }
            break;
        
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
                    size = 0x400000;
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
                    size = 0x80000;
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
