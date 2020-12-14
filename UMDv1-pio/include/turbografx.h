/*******************************************************************//**
 *  \file turbografx.h
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

#ifndef turbografx_h
#define turbografx_h
#include "umdv1.h"
#include "stdint.h"

/*******************************************************************//** 
 * \class turbografx
 * \brief turbografx specific methods
 **********************************************************************/
class turbografx: public umdv1
{
    public:
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        turbografx();
        
        virtual void setup(uint8_t param);
        
        /*******************************************************************//**
         * \brief Perform checksum operations on the cartridge
         * \return void
         **********************************************************************/
		virtual void calcChecksum();
        
        virtual void enableSram(uint8_t param);

        virtual void disableSram(uint8_t param);

        /**
         * \brief There's no way to know the rom size, so this
         *        returns 0
         * \return 0
         */        
        virtual uint32_t getRomSize();

    private:

        //Master System pin functions
        static const uint8_t TG16_nRST = 38;

};

#endif
