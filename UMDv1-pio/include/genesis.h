/*******************************************************************//**
 *  \file genesis.h
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

#ifndef genesis_h
#define genesis_h
#include "umdbase.h"

/*******************************************************************//** 
 * \class genesis
 * \brief Genesis specific methods
 **********************************************************************/
class genesis: public umdbase
{
    public:
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        genesis();
        
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
         * \param wait specify whether to wait for the operation to complete
         * \return void
         **********************************************************************/
        virtual void eraseChip(bool wait);
        
        /*******************************************************************//**
         * \brief Write a byte to a 24bit address on the odd byte
         * \param address 24bit address
         * \param data byte
         * \return void
         **********************************************************************/
        virtual void writeByte(uint32_t address, uint8_t data);
        
        /*******************************************************************//**
         * \brief Write a byte to the nTIME region on genesis
         * \param address 32bit address
         * \param data byte
         * \return void
         **********************************************************************/
        void writeByteTime(uint32_t address, uint8_t data);
        
        void programWordBuffer(uint32_t address, uint16_t * buf, uint8_t size);

        /*******************************************************************//**
         * \brief Read a word without converting to littel endian
         * \param address 32bit address
         * \return word big endian word
         **********************************************************************/
        uint16_t readBigWord(uint32_t address);
        
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
    
        //Genesis pin functions
        static const uint8_t GEN_SL1 = 38;
        static const uint8_t GEN_SR1 = 39;
        static const uint8_t GEN_nDTACK = 40;
        static const uint8_t GEN_nCAS2 = 41;
        static const uint8_t GEN_nVRES = 42;
        static const uint8_t GEN_nLWR = 43;
        static const uint8_t GEN_nUWR = 44;
        static const uint8_t GEN_nTIME = 45;
};

#endif
