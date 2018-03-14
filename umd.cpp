/*******************************************************************//**
 *  \file umd.cpp
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
#include "umd.h"

/*******************************************************************//**
 * The constructor sets the mode using setMode() to undefined.
 **********************************************************************/
umd::umd() 
{
    setMode(undefined);
}

/*******************************************************************//**
 * The _setDatabusInput function sets the databus to inputs and 
 * deactivates the built-in pullup resistors
 **********************************************************************/
inline void umd::_setDatabusInput()
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
inline void umd::_setDatabusOutput()
{
    //set data to outputs
    DATAH_DDR = 0xFF;
    DATAL_DDR = 0xFF;
}

/*******************************************************************//**
 * The resetCart() function issues a 250ms active-low reset pulse to
 * the pin specified by the variable _resetPin.
 * 
  **********************************************************************/
void umd::resetCart()
{
    digitalWrite(_resetPin, LOW);
    delay(200);
    digitalWrite(_resetPin, HIGH);
    delay(200);
}

/*******************************************************************//**
 * The detectCart() functions tests if the nCART line is pulled low
 * by a cartridge.
 * 
 * \warning incompatible with Colecovision mode
 **********************************************************************/
bool umd::detectCart()
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
 * selected cartridge. The _mode and _resetPin variable store the
 * current mode and resetPin numbers for later use by the firmware.
 **********************************************************************/
void umd::setMode(eMode mode)
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

    pinMode(CTRL0, INPUT);
    pinMode(CTRL1, INPUT);
    pinMode(CTRL2, INPUT);
    pinMode(CTRL3, INPUT);
    pinMode(CTRL4, INPUT);
    pinMode(CTRL5, INPUT);
    pinMode(CTRL6, INPUT);
    pinMode(CTRL7, INPUT);

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
            
        case PC:
            pinMode(TG_nRST, OUTPUT);
            digitalWrite(TG_nRST, HIGH);
            _resetPin = TG_nRST;
            resetCart();
            _mode = PC;
            break;
            
        case TG:
            pinMode(TG_nRST, OUTPUT);
            digitalWrite(TG_nRST, HIGH);
            _resetPin = TG_nRST;
            resetCart();
            _mode = TG;
            break;
            
        case CV:
            _resetPin = 45; //unused with coleco
            _mode = CV;
            break;
            
        case MS:
            pinMode(SMS_nRST, OUTPUT);
            digitalWrite(SMS_nRST, HIGH);
            _resetPin = SMS_nRST;
            resetCart();
            _mode = MS;
            break;
            
        case SN:
            pinMode(SN_nRST, OUTPUT);
            digitalWrite(SN_nRST, HIGH);
            _resetPin = SN_nRST;
            resetCart();
            _mode = SN;
            break;
            
        default:
            //control signals default to all inputs

            _mode = undefined;
            break;
    }
}

/*******************************************************************//**
 * The _latchAddress function latches a 24bit address to the cartridge
 * \warning incompatible with Colecovision mode
 **********************************************************************/
inline void umd::_latchAddress(uint32_t address)
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
    digitalWrite(ALE_low, HIGH);
    digitalWrite(ALE_low, LOW);

    //put high address on bus and latch it
    DATAOUTH = 0x00;
    DATAOUTL = addrh;
    digitalWrite(ALE_high, HIGH);
    digitalWrite(ALE_high, LOW);
    
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
inline void umd::_latchAddress(uint16_t address)
{
    uint8_t addrm,addrl;
    
    //separate address into 2 bytes for address latches
    addrl = (uint8_t)(address & 0xFF);
    addrm = (uint8_t)(address>>8 & 0xFF);

    _setDatabusOutput();

    //put low and mid address on bus and latch it
    DATAOUTH = addrm;
    DATAOUTL = addrl;
    digitalWrite(ALE_low, HIGH);
    digitalWrite(ALE_low, LOW);
    
    _setDatabusInput();
}

/*******************************************************************//**
 * The getFlashID() function returns the manufacturer and product ID from 
 * the Flash IC as a uint16_t. It automatically performs the correct
 * flash ID read sequence based on the currently selected mode.
 **********************************************************************/
uint32_t umd::getFlashID()
{
    uint32_t flashID = 0;

    switch(_mode)
    {
        //mx29f800 software ID detect word mode
        //A1 of Dumper connected to A0 of MX29F800
        //Automatic big endian on word mode
        case MD:
            //writeWord(0x00000555, 0xAA00);
            //writeWord(0x000002AA, 0x5500);
            //writeWord(0x00000555, 0x9000);
            //flashID = readWord(0x00000001);
            //writeWord(0x00000000, 0xF000);
            writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
            writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
            writeWord( (uint32_t)(0x000555 << 1), 0x9000);
            flashID = (uint32_t)readWord( (uint16_t)(0x000001 << 1) );
            
            //exit software ID
            writeWord( (uint32_t)0x000000, 0xF000);
            
            flashID <<= 16;
            
            //try for second chip
            writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0xAA00);
            writeWord( (uint32_t)(0x0002AA << 1) + GEN_CHIP_1_BASE, 0x5500);
            writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0x9000);
            flashID |= (uint32_t)readWord( (0x000001 << 1) + GEN_CHIP_1_BASE );
            
            //exit software ID
            writeWord( (uint32_t)0x000000  + GEN_CHIP_1_BASE, 0xF000);
            
            _flashID = flashID;
            
            break;
        //mx29f800 software ID detect byte mode
        case PC:
        case SN:
        case TG:
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x90);
            flashID = (uint32_t)readByte((uint16_t)0x0000, false);
            flashID <<= 8;
            //exit software ID
            writeByte((uint16_t)0x0000, 0xF0);
            
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x90);
            flashID |= (uint32_t)readByte((uint16_t)0x0001, false);
            
            //exit software ID
            writeByte((uint16_t)0x0000, 0xF0);
            
            _flashID = flashID;
            break;
        //mx29f800 software ID detect byte mode through SMS mapper
        case MS:
            //enable rom write enable bit
            writeByte((uint16_t)SMS_CONF_REG_ADDR,0x80);
            
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x90);
            flashID = (uint32_t)readByte((uint16_t)0x0000, false);
            flashID <<= 8;
            //exit software ID
            writeByte((uint16_t)0x0000, 0xF0);
            
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x90);
            flashID |= (uint32_t)readByte((uint16_t)0x0001, false);
            
            //exit software ID
            writeByte((uint16_t)0x0000,0xF0);
            
            _flashID = flashID;
            
            //disable rom write enable bit
            writeByte((uint16_t)SMS_CONF_REG_ADDR,0x00);
            break;  
        //SST39SF0x0 software ID detect
        case CV:
            writeByte((uint16_t)0x5555,0xAA);
            writeByte((uint16_t)0x2AAA,0x55);
            writeByte((uint16_t)0x5555,0x90);
            
            flashID = (uint32_t)readByte((uint16_t)0x0000, false);
            flashID <<= 8;
            flashID |= (uint32_t)readByte((uint16_t)0x0001, false);
            
            //exit software ID
            writeByte((uint16_t)0x0000,0xF0);
            
            _flashID = flashID;
            break;
        default:
            flashID = 0xFFFFFFFF;
        break;
    }

    return flashID;
}

/*******************************************************************//**
 * The eraseChip() function erases the entire flash. If the wait parameter
 * is true the function will block with toggle bit until the erase 
 * operation has completed. It will return the time in millis the
 * operation required to complete.
 **********************************************************************/
uint32_t umd::eraseChip(bool wait, uint8_t chip)
{
    uint32_t startMillis, intervalMillis;
    
    switch(_mode)
    {
        //mx29f800 chip erase word mode
        case MD:
            if (chip == 0)
            {
                writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
                writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
                writeWord( (uint32_t)(0x000555 << 1), 0x8000);
                writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
                writeWord( (uint32_t)(0x0002AA << 1), 0x5500);
                writeWord( (uint32_t)(0x000555 << 1), 0x1000);
            }else if (chip == 1)
            {
                writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0xAA00);
                writeWord( (uint32_t)(0x0002AA << 1) + GEN_CHIP_1_BASE, 0x5500);
                writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0x8000);
                writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0xAA00);
                writeWord( (uint32_t)(0x0002AA << 1) + GEN_CHIP_1_BASE, 0x5500);
                writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0x1000);                
            }
            break;
        //mx29f800 chip erase byte mode through SMS mapper
        case MS:
            //enable rom write enable bit
            writeByte((uint16_t)SMS_CONF_REG_ADDR,0x80);
            
            //set proper slot registers, slot 0 and 1 needed for flash ID
            setSMSSlotRegister(0,0x0000);
            setSMSSlotRegister(1,0x4000);
            setSMSSlotRegister(2,0x8000);
            
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x80);
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x10);
            
            //disable rom write enable bit
            writeByte((uint16_t)SMS_CONF_REG_ADDR,0x00);
            
            break;
        case PC:
        case SN:
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
        intervalMillis = startMillis;
        
        // wait for 4 consecutive toggle bit success reads before exiting
        while( toggleBit(4, chip) != 4 )
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
        return ( millis() - startMillis );
        
    }else
    {
        return 0;
    }
}

/*******************************************************************//**
 * The eraseSector function erases a sector in the flash memory. The
 * Address specified can be any address in the sector.
 **********************************************************************/
void umd::eraseSector(bool wait, uint32_t sectorAddress)
{
    switch(_mode)
    {
        case MD:
            //mx29f800 sector erase word mode
            writeWord((uint16_t)(0x0555 << 1), 0xAA00);
            writeWord((uint16_t)(0x02AA << 1), 0x5500);
            writeWord((uint16_t)(0x0555 << 1), 0x8000);
            writeWord((uint16_t)(0x0555 << 1), 0xAA00);
            writeWord((uint16_t)(0x02AA << 1), 0x5500);
            writeWord((uint32_t)sectorAddress, 0x3000);
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
            }
            break;
        case TG:
            //mx29f800 sector erase byte mode
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0x80);
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint32_t)sectorAddress, 0x30);
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
            }
            break;
            break;
        case CV:
            //SST39SF0x0 sector erase
            writeByte((uint16_t)0x5555, 0xAA);
            writeByte((uint16_t)0x2AAA, 0x55);
            writeByte((uint16_t)0x5555, 0x80);
            writeByte((uint16_t)0x5555, 0xAA);
            writeByte((uint16_t)0x2AAA, 0x55);
            writeByte((uint32_t)sectorAddress, 0x30);
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
            }
            break;
            break;
        default:
            break;
    }
}

/*******************************************************************//**
 * The readByte(uint16_t) function returns a byte read from 
 * a 16bit address.
 * 
 * \warning in PC Engine mode, reads from outside this class need to
 * reverse the databus for consistency.
 **********************************************************************/
uint8_t umd::readByte(uint16_t address, bool external)
{
    uint8_t readData;

    _latchAddress(address);
    _setDatabusInput();

    // read the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nRD, LOW);
  
    switch(_mode)
    {
        case PC:
            if( external )
            {
                readData = reverseByte(DATAINL);
            }else
            {
                readData = DATAINL;
            }
            break;
        case MS:
        case MD:
        case TG:
        case SN:
        case CV:
        default:
            readData = DATAINL;
            break;
    }
  
    digitalWrite(nCE, HIGH);
    digitalWrite(nRD, HIGH);

    return readData;
}

/*******************************************************************//**
 * The readByte(uint32_t) function returns a byte read from 
 * a 24bit address.
 * 
 * \warning in PC Engine mode, reads from outside this class need to
 * reverse the databus for consistency.
 **********************************************************************/
uint8_t umd::readByte(uint32_t address, bool external)
{
    uint8_t readData;

    _latchAddress(address);
    _setDatabusInput();

    // read the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nRD, LOW);
  
    switch(_mode)
    {
        case PC:
            if( external )
            {
                readData = reverseByte(DATAINL);
            }else
            {
                readData = DATAINL;
            }
            break;
        case MS:
        case MD:
        case TG:
        case SN:
        case CV:
        default:
            readData = DATAINL;
            break;
    }
  
    digitalWrite(nCE, HIGH);
    digitalWrite(nRD, HIGH);

    return readData;
}

/*******************************************************************//**
 * The readWord(uint32_t) function returns a word read from 
 * a 24bit address.
 * 
 * \warning converts to little endian
 **********************************************************************/
uint16_t umd::readWord(uint32_t address)
{
    //only genesis mode reads words

    uint16_t readData;

    _latchAddress(address);
    _setDatabusInput();

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
 * The writeByteTime function strobes a byte into nTIME region
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umd::writeByteTime(uint16_t address, uint8_t data)
{
    _latchAddress(address);
    _setDatabusOutput();

    //put byte on bus
    DATAOUTL = data;
    
    // write to the bus
    digitalWrite(GEN_nTIME, LOW);
    delayMicroseconds(1);
    digitalWrite(GEN_nTIME, HIGH);
 
    _setDatabusInput();
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 16bit
 * address. The upper 8 address bits (23..16) are not modified
 * by this function so this can be used to perform quicker successive
 * writes within a 64k boundary.
 * 
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umd::writeByte(uint16_t address, uint8_t data)
{
    _latchAddress(address);
    _setDatabusOutput();

    //write genesis odd bytes to the high byte of the bus
    switch(_mode)
    {
        case MD:
            DATAOUTL = data;
            // write to the bus
            digitalWrite(nCE, LOW);
            digitalWrite(GEN_nLWR, LOW);
            delayMicroseconds(1);
            digitalWrite(GEN_nLWR, HIGH);
            digitalWrite(nCE, HIGH);
            break;
        case MS:
        case PC:
        case TG:
        case SN:
        case CV:
        default:
            DATAOUTL = data;
            // write to the bus
            digitalWrite(nCE, LOW);
            digitalWrite(nWR, LOW);
            delayMicroseconds(1);
            digitalWrite(nWR, HIGH);
            digitalWrite(nCE, HIGH);
            break;
    }
  
    _setDatabusInput();
    
}

/*******************************************************************//**
 * The writeByte function strobes a byte into the cartridge at a 24bit
 * address.
 **********************************************************************/
void umd::writeByte(uint32_t address, uint8_t data)
{
    _latchAddress(address);
    _setDatabusOutput();
    
    switch(_mode)
    {
        case MD:
            DATAOUTL = data;
            // write to the bus
            digitalWrite(nCE, LOW);
            digitalWrite(GEN_nLWR, LOW);
            delayMicroseconds(1);
            digitalWrite(GEN_nLWR, HIGH);
            digitalWrite(nCE, HIGH);
            break;
        case MS:
            DATAOUTL = data;
            // write to the bus
            digitalWrite(nCE, LOW);
            digitalWrite(nWR, LOW);
            delayMicroseconds(2);
            digitalWrite(nWR, HIGH);
            digitalWrite(nCE, HIGH);
            break;
        case PC:
        case TG:
        case SN:
        case CV:
        default:
            DATAOUTL = data;
            // write to the bus
            digitalWrite(nCE, LOW);
            digitalWrite(nWR, LOW);
            delayMicroseconds(1);
            digitalWrite(nWR, HIGH);
            digitalWrite(nCE, HIGH);
            break;
    }
    
    _setDatabusInput();
    
}

/*******************************************************************//**
 * The writeGenesisTime function strobes a word into nTIME region
 * 
 * \warning word is converted to big endian
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umd::writeWordTime(uint16_t address, uint16_t data)
{
    _latchAddress(address);
    _setDatabusOutput();

    //put word on bus
    DATAOUTH = (uint8_t)(data);
    DATAOUTL = (uint8_t)(data>>8);
    
    // write to the bus
    digitalWrite(GEN_nTIME, LOW);
    //delayMicroseconds(1);
    digitalWrite(GEN_nTIME, HIGH);
  
    _setDatabusInput();
}

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 24bit
 * address.
 * 
 * \warning word is converted to big endian
 **********************************************************************/
void umd::writeWord(uint32_t address, uint16_t data)
{
    _latchAddress(address);
    _setDatabusOutput();

    //put word on bus
    DATAOUTH = (uint8_t)(data);
    DATAOUTL = (uint8_t)(data>>8);

    // write to the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nWR, LOW);
    //delayMicroseconds(1);
    digitalWrite(nWR, HIGH);
    digitalWrite(nCE, HIGH);
  
    _setDatabusInput();
}

/*******************************************************************//**
 * The writeWord function strobes a word into the cartridge at a 16bit
 * address.
 * 
 * \warning word is converted to big endian
 * \warning upper 8 address bits (23..16) are not modified
 **********************************************************************/
void umd::writeWord(uint16_t address, uint16_t data)
{
    _latchAddress(address);
    _setDatabusOutput();

    //put word on bus
    DATAOUTH = (uint8_t)(data);
    DATAOUTL = (uint8_t)(data>>8);

    // write to the bus
    digitalWrite(nCE, LOW);
    digitalWrite(nWR, LOW);
    //delayMicroseconds(1);
    digitalWrite(nWR, HIGH);
    digitalWrite(nCE, HIGH);
  
    _setDatabusInput();
}

/*******************************************************************//**
 * The programByte function programs a byte into the flash array at a
 * 24bit address. If the wait parameter is true the function will wait 
 * for the program operation to complete using data polling before 
 * returning.
 * 
 * \warning Sector or entire IC must be erased prior to programming
 **********************************************************************/
void umd::programByte(uint32_t address, uint8_t data, bool wait)
{
    switch(_mode)
    {
        //MX29F800 program byte with reversed data
        case PC:
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0xA0);
            writeByte(address, reverseByte(data));
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
            }
            break;
        //MX29F800 program byte
        case SN:
        case TG:
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0xA0);
            writeByte(address, data);
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
            }
            break;
        //MX29F800 program byte through SMS mapper
        case MS:
            //enable rom write enable bit
            writeByte((uint16_t)SMS_CONF_REG_ADDR,0x80);
        
            writeByte((uint16_t)0x0AAA, 0xAA);
            writeByte((uint16_t)0x0555, 0x55);
            writeByte((uint16_t)0x0AAA, 0xA0);
            
            //program byte in through slot 2
            writeByte(setSMSSlotRegister(2,address), data);
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
            }
            
            //disable rom write enable bit
            writeByte((uint16_t)SMS_CONF_REG_ADDR,0x00);
            break;
        //SST39SF0x0 program byte
        case CV:
            writeByte((uint16_t)0x5555, 0xAA);
            writeByte((uint16_t)0x2AAA, 0x55);
            writeByte((uint16_t)0x5555, 0xA0);
            
            writeByte(address, data);
            
            //use data polling to validate end of program cycle
            if(wait)
            {
                while( toggleBit(2, 0) != 2 );
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
 * \warning Sector or entire IC must be erased prior to programming
 **********************************************************************/
void umd::programWord(uint32_t address, uint16_t data, bool wait)
{
    switch(_mode)
    {
        //MX29F800 program word
        case MD:
            //Genesis MDCart can have multiple flash ICs
            if ( address < GEN_CHIP_1_BASE )
            {
                writeWord( (uint32_t)(0x000555 << 1), 0xAA00);
                writeWord( (uint16_t)(0x0002AA << 1), 0x5500);
                writeWord( (uint16_t)(0x000555 << 1), 0xA000);
                writeWord( (uint32_t)address, data );
                //use toggle bit to validate end of program cycle
                if(wait)
                {
                    while( toggleBit(2, 0) != 2 );
                }
            }else
            {
                writeWord( (uint32_t)(0x000555 << 1) + GEN_CHIP_1_BASE, 0xAA00);
                writeWord( (uint16_t)(0x0002AA << 1), 0x5500);
                writeWord( (uint16_t)(0x000555 << 1), 0xA000);
                writeWord( (uint32_t)address, data );
                //use toggle bit to validate end of program cycle
                if(wait)
                {
                    while( toggleBit(2, 1) != 2 );
                }
            }
            
            break;
        default:
            break;
    }
}

/*******************************************************************//**
 * The toggleBit uses the toggle bit flash algorithm to determine if
 * the current program operation has completed
 **********************************************************************/
uint8_t umd::toggleBit(uint8_t attempts, uint8_t chip)
{
    uint8_t retValue = 0;
    uint16_t read16Value, old16Value;
    uint8_t readValue, oldValue;
    uint8_t i;
    
    uint32_t baseAddress = 0;
    
    switch(_mode)
    {
        //mx29f800 toggle bit on bit 6
        case MD:
            if( chip == 1 )
            {
                baseAddress =  + GEN_CHIP_1_BASE;
            }
            
            //first read of bit 6 - big endian
            old16Value = readWord((uint32_t)baseAddress) & 0x4000;
            
            for( i=0; i<attempts; i++ )
            {
                //successive reads compare this read to the previous one for toggle bit
                read16Value = readWord((uint32_t)baseAddress) & 0x4000;
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
            
        //mx29f800 toggle bit on bit 6
        case MS:
        case PC:
        case SN:
        case TG:
            //first read of bit 6
            oldValue = readByte((uint16_t)0x0000, false) & 0x40;
            
            for( i=0; i<attempts; i++ )
            {
                //successive reads compare this read to the previous one for toggle bit
                readValue = readByte((uint16_t)0x0000, false) & 0x40;
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
        //SST39SF0x0 toggle bit on bit 6
        case CV:
            
            //first read should always be a 1 according to datasheet
            oldValue = readByte((uint16_t)0x0000, false) & 0x40;
            
            for( i=0; i<attempts; i++ )
            {
                //successive reads compare this read to the previous one for toggle bit
                readValue = readByte((uint16_t)0x0000, false) & 0x40;
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

/*******************************************************************//**
 * The reverseByte function uses a table to reverse the bits in a byte
 * Useful for reading reverse databus on PC Engine
 **********************************************************************/
uint8_t umd::reverseByte(uint8_t data)
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

/*******************************************************************//**
 * The setSMSSlotRegister function updates the cartridge slot register
 * with the correct bank number of the corresponding address
 **********************************************************************/
uint16_t umd::setSMSSlotRegister(uint8_t slotNum, uint32_t address)
{
    uint16_t virtualAddress;
    
    switch( slotNum )
    {
        case 0:
            writeByte( SMS_SLOT_0_REG_ADDR, (uint8_t)(address >> 14) );
            virtualAddress = ( SMS_SLOT_0_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
        case 1:
            writeByte( SMS_SLOT_1_REG_ADDR, (uint8_t)(address >> 14) );
            virtualAddress = ( SMS_SLOT_1_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
        case 2:
            writeByte( SMS_SLOT_2_REG_ADDR, (uint8_t)(address >> 14) );
            virtualAddress = ( SMS_SLOT_2_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
        default:
            writeByte( SMS_SLOT_2_REG_ADDR, (uint8_t)(address >> 14) );
            virtualAddress = ( SMS_SLOT_2_ADDR | ( (uint16_t)address & 0x3FFF) );
            break;
    }
    return virtualAddress;
}
