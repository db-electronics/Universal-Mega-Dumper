/*******************************************************************//**
 *  \file sms.h
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

#ifndef sms_h
#define sms_h

/*******************************************************************//** 
 * \class genesis
 * \brief Genesis specific methods
 **********************************************************************/
class sms: public umdbase
{
    public:
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        sms();
        
        virtual void setup();
        
        /*******************************************************************//**
         * \brief Read the Manufacturer and Product ID in the Flash IC
         * \param alg The algorithm to use, SST 5V devices are different
         * \return void
         **********************************************************************/
        virtual void getFlashID(uint8_t alg);
        
        /*******************************************************************//**
         * \brief Perform checksum operations on the cartridge
         * \return void
         **********************************************************************/
		virtual void calcChecksum();
        
        /*******************************************************************//**
         * \brief Read a byte from a 24bit address using mapper
         * \param address 24bit address
         * \return byte from cartridge
         **********************************************************************/
        virtual uint8_t readByte(uint32_t address);

	virtual uint16_t readWord(uint32_t address);
        
        /*******************************************************************//**
         * \brief set the SMS slot register value
         * \param slotNum the slot number
         * \param address the real ROM address
         * \return virtual address
         **********************************************************************/
        uint16_t setSMSSlotRegister(uint8_t slotNum, uint32_t address);
        
        /*******************************************************************//**
         * \brief Enable reading and writing to the SRAM
         * \param param none for Genesis
         * \return void
         **********************************************************************/
        virtual void enableSram(uint8_t param);
        
        /*******************************************************************//**
         * \brief Disable reading and writing to the SRAM
         * \param param none for Genesis
         * \return void
         **********************************************************************/
        virtual void disableSram(uint8_t param);
        
        
    private:

        static const uint16_t SMS_SLOT_0_ADDR = 0x0000;     ///< SMS Sega Mapper slot 0 base address 0x0000 - 0x3FFF
        static const uint16_t SMS_SLOT_1_ADDR = 0x4000;     ///< SMS Sega Mapper slot 1 base address 0x4000 - 0x7FFF
        static const uint16_t SMS_SLOT_2_ADDR = 0x8000;     ///< SMS Sega Mapper slot 2 base address 0x8000 - 0xBFFF
        static const uint16_t SMS_CONF_REG_ADDR   = 0xFFFC; ///< SMS Sega Mapper RAM mapping and miscellaneous functions register
        static const uint16_t SMS_SLOT_0_REG_ADDR = 0xFFFD; ///< SMS Sega Mapper slot 0 register address 0x0000 - 0x3FFF
        static const uint16_t SMS_SLOT_1_REG_ADDR = 0xFFFE; ///< SMS Sega Mapper slot 1 register address 0x4000 - 0x7FFF
        static const uint16_t SMS_SLOT_2_REG_ADDR = 0xFFFF; ///< SMS Sega Mapper slot 2 register address 0x8000 - 0xBFFF
        
        uint8_t SMS_SelectedPage = 0xFF;
    
        //Master System pin functions
        static const uint8_t SMS_nRST = 42;
};

#endif
