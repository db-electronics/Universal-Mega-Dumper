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
 * \class sms
 * \brief sms specific methods
 **********************************************************************/
class sms: public umdbase
{
    public:
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        sms();
        
        virtual void setup(uint8_t alg);
        
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
         * \brief Get the ROM's size
         * \return the ROM's size
         **********************************************************************/
		virtual uint32_t getRomSize();
        
        /*******************************************************************//**
         * \brief Erase the entire Flash IC
         * \param wait specify whether to wait for the operation to complete before returning
         * \return void
         **********************************************************************/
        virtual void eraseChip(bool wait);

        /*******************************************************************//**
         * \brief Read a byte from a 24bit address using mapper
         * \param address 24bit address
         * \return byte from cartridge
         **********************************************************************/
        virtual uint8_t readByte(uint32_t address);

        /*******************************************************************//**
         * \brief Write a byte to a 24bit address
         * \param address 24bit address
         * \param data byte
         * \return void
         **********************************************************************/
        virtual void writeByte(uint32_t address, uint8_t data);

        /*******************************************************************//**
         * \brief Program a byte in the Flash IC
         * \param address 24bit address
         * \param data byte
         * \param wait Wait for completion using toggle bit to return from function
         * \return void
         **********************************************************************/
        virtual void programByte(uint32_t address, uint8_t data, bool wait);

        /*******************************************************************//**
         * \brief set the SMS slot register value
         * \param slotNum the slot number
         * \param address the real ROM address
         * \return virtual address
         **********************************************************************/
        uint16_t setSMSSlotRegister(uint8_t slotNum, uint32_t address);
        
        /*******************************************************************//**
         * \brief Enable reading and writing to the SRAM
         * \param param none for SMS
         * \return void
         **********************************************************************/
        virtual void enableSram(uint8_t param);
        
        /*******************************************************************//**
         * \brief Disable reading and writing to the SRAM
         * \param param none for SMS
         * \return void
         **********************************************************************/
        virtual void disableSram(uint8_t param);
        
        void romWrites(bool enable);
        
    private:

        static const uint16_t SMS_SLOT_MASK = 0x3FFF;
        static const uint16_t SMS_SLOT_0_ADDR = 0x0000;     ///< SMS Sega Mapper slot 0 base address 0x0000 - 0x3FFF
        static const uint16_t SMS_SLOT_1_ADDR = 0x4000;     ///< SMS Sega Mapper slot 1 base address 0x4000 - 0x7FFF
        static const uint16_t SMS_SLOT_2_ADDR = 0x8000;     ///< SMS Sega Mapper slot 2 base address 0x8000 - 0xBFFF
        static const uint16_t SMS_CONF_REG_ADDR   = 0xFFFC; ///< SMS Sega Mapper RAM mapping and miscellaneous functions register
        static const uint16_t SMS_SLOT_0_REG_ADDR = 0xFFFD; ///< SMS Sega Mapper slot 0 register address 0x0000 - 0x3FFF
        static const uint16_t SMS_SLOT_1_REG_ADDR = 0xFFFE; ///< SMS Sega Mapper slot 1 register address 0x4000 - 0x7FFF
        static const uint16_t SMS_SLOT_2_REG_ADDR = 0xFFFF; ///< SMS Sega Mapper slot 2 register address 0x8000 - 0xBFFF
        
        uint8_t SMS_SelectedPage = 0xFF;
    
        uint32_t skipChecksumStart, skipChecksumEnd;
    
        //Master System pin functions
        static const uint8_t SMS_nRST = 42;

};

#endif
