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

#ifndef cartfactory_h
#define cartfactory_h

#include <stdint.h>
#include "umdbase.h"



class CartFactory 
{
public:
    CartFactory();
#define CARTS_LEN  7
    enum Mode : uint8_t { UNDEFINED, COLECO, GEN, SMS, PCE, TG16, SNES, SNESLO };

    umdbase* getCart(Mode mode);
    Mode getMaxCartMode();

    ~CartFactory();

private:
    CartFactory(const CartFactory&) = delete;

    // Array of carts indexed by Mode
    umdbase* carts[CARTS_LEN+1];

    umdbase* noop;
};

#endif


