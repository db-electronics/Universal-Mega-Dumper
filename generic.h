/*******************************************************************//**
 *  \file generic.h
 *  \author btrim
 *  \brief This program allows to read and write to various game cartridges. 
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

#ifndef generic_h
#define generic_h
#include <stdint.h>
#include "umdbase.h"
/*******************************************************************//** 
 * \class Generic
 * \brief Teensy umd class to read and write db Flash Carts
 * Relies on umdbase for all operations but provides no-op methods for 
 * umdbase's pure virtual functions.
 **********************************************************************/
class GenericCart : public umdbase
{
    virtual void calcChecksum();

    virtual void enableSram(uint8_t param);

    virtual void disableSram(uint8_t param);

    /**
     * \brief There's no way to know the rom size, so this
     *        returns 0
     * \return 0
     */        
    virtual uint32_t getRomSize();
};

#endif
