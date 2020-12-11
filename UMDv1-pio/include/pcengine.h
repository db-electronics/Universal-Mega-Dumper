/*******************************************************************//**
 *  \file pcengine.h
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

#ifndef pcengine_h
#define pcengine_h
#include "umdbase.h"
#include "stdint.h"

/*******************************************************************//** 
 * \class pcengine
 * \brief pcengine specific methods
 **********************************************************************/
class pcengine: public umdbase
{
    public:
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        pcengine();
        
        virtual void setup(uint8_t alg);
        
        /*******************************************************************//**
         * \brief Perform checksum operations on the cartridge
         * \return void
         **********************************************************************/
		virtual void calcChecksum();
        
    private:
    
        //Master System pin functions
        static const uint8_t SMS_nRST = 42;

};

#endif
