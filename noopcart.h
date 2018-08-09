/*******************************************************************//**
 *  \file noopcart.h
 *  \author btrim
 *  \brief This program allows to read and write to various game cartridges. 
 *         The UMD base class handles all generic cartridge operations, console
 *         specific operations are handled in derived classes.
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

#ifndef noopcart_h
#define noopcart_h

/*******************************************************************//** 
 * \class umdbase
 * \brief Teensy umd class to read and write db Flash Carts
 **********************************************************************/
class noopcart : public umdbase
{
    public:
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        noopcart(){}
        
        /*******************************************************************//**
         * \brief setup the UMD's hardware for the current cartridge
         * \return void
         **********************************************************************/
        virtual void setup(){}
        
        /*******************************************************************//**
         * \name Cartridge Flash Functions
         * This group of functions is specific to the catridge flash
         **********************************************************************/
        /**@{*/
        /*******************************************************************//**
         * \brief Read the Manufacturer and Product ID in the Flash IC
         * \param alg The algorithm to use, SST 5V devices are different
         * \return void
         **********************************************************************/
        virtual void getFlashID(uint8_t alg){}
        
        /*******************************************************************//**
         * \brief Erase the entire Flash IC
         * \param wait specify whether to wait for the operation to complete before returning
         * \return void
         **********************************************************************/
        virtual void eraseChip(bool wait){}
        
        /*******************************************************************//**
         * \brief Perform toggle bit algorithm byte mode
         * \param attempts how many toggle bits to attempt
         * \return the number of times the bit successfully toggled
         **********************************************************************/
        uint8_t toggleBit8(uint8_t attempts){return 0;}

		/*******************************************************************//**
         * \brief Perform toggle bit algorithm word mode
         * \param attempts how many toggle bits to attempt
         * \return the number of times the bit successfully toggled
         **********************************************************************/
        uint8_t toggleBit16(uint8_t attempts){return 0;}


		/*******************************************************************//**
         * \brief Perform checksum on the cartridge
         * \return the calculated value
         **********************************************************************/
		virtual void calcChecksum(){}

		/*******************************************************************//**
         * \brief enable SRAM memory access
         * \param param any paramaters to pass, not used by all cartridges
         * \return void
         **********************************************************************/
		virtual void enableSram(uint8_t param){}
		
		/*******************************************************************//**
         * \brief disable SRAM memory access
         * \param param any paramaters to pass, not used by all cartridges
         * \return void
         **********************************************************************/
		virtual void disableSram(uint8_t param){};

        /**@}*/
        
        /*******************************************************************//**
         * \name Read Functions
         * This group of functions perform various read operations
         **********************************************************************/
        /**@{*/
        /*******************************************************************//**
         * \brief Read a byte from a 16bit address
         * \param address 16bit address
         * \return byte from cartridge
         **********************************************************************/
        virtual uint8_t readByte(uint16_t address){return 0;};
        
        /*******************************************************************//**
         * \brief Read a byte from a 24bit address
         * \param address 24bit address
         * \return byte from cartridge
         **********************************************************************/
        virtual uint8_t readByte(uint32_t address){return 0;};
        
        /*******************************************************************//**
         * \brief Read a word from a 24bit address
         * \param address 24bit address
         * \return word from cartridge
         **********************************************************************/
        virtual uint16_t readWord(uint32_t address){return 0;}
        
        /**@}*/
        
        
        /*******************************************************************//**
         * \name Write Functions
         * This group of functions perform various write operations
         **********************************************************************/
        /**@{*/
        
        /*******************************************************************//**
         * \brief Write a byte to a 16bit address
         * \param address 16bit address
         * \param data byte
         * \return void
         **********************************************************************/
        virtual void writeByte(uint16_t address, uint8_t data){}
        
        /*******************************************************************//**
         * \brief Write a byte to a 24bit address
         * \param address 24bit address
         * \param data byte
         * \return void
         **********************************************************************/
        virtual void writeByte(uint32_t address, uint8_t data){}

	virtual void writeByteTime(uint32_t address, uint8_t data){}
        
        /*******************************************************************//**
         * \brief Write a word to a 24bit address
         * \param address 24bit address
         * \param data word
         * \return void
         **********************************************************************/
        virtual void writeWord(uint32_t address, uint16_t data){}
        
        
        /**@}*/
        
        /*******************************************************************//** 
         * \name Program Functions
         * This group of functions perform various write operations
         **********************************************************************/
        /**@{*/
        /*******************************************************************//**
         * \brief Program a byte in the Flash IC
         * \param address 24bit address
         * \param data byte
         * \param wait Wait for completion using toggle bit to return from function
         * \return void
         **********************************************************************/
        virtual void programByte(uint32_t address, uint8_t data, bool wait){}
        
        /*******************************************************************//**
         * \brief Program a word in the Flash IC
         * \param address 24bit address
         * \param data word
         * \param wait Wait for completion using toggle bit to return from function
         * \return void
         **********************************************************************/
        virtual void programWord(uint32_t address, uint16_t data, bool wait){}
    
        /**@}*/
        
    	/*******************************************************************//**
         * \brief latch a 16bit address
         * \return void
         **********************************************************************/
        void latchAddress(uint16_t address){}
        
        /*******************************************************************//**
         * \brief latch a 24bit address
         * \return void
         **********************************************************************/
        void latchAddress(uint32_t address){}
                
        /*******************************************************************//**
         * \brief Read the Manufacturer and Product ID in the Flash IC
         * \param manufacturer the byte specifying the manufacturer
         * \param device the byte specifying the device
         * \param info the byte specifying additional info
         * \return size the size of the flash in bytes
         **********************************************************************/
        uint32_t getFlashSizeFromID(uint8_t manufacturer, uint8_t device, uint8_t info){return 0;}
};

#endif
