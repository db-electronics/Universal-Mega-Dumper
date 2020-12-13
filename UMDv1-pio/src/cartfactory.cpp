/*******************************************************************//**
 *  \file cartfactory.cpp
 *  \author btrim
 *  \brief This program provides a serial interface over USB to the
 *         Universal Mega Dumper. 
 *
 *  \copyright This file is part of Universal Mega Dumper.
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
 
#include <stdint.h>
#include "umdbase.h"
#include "cartfactory.h"
#include "genesis.h"
#include "sms.h"
#include "pcengine.h"
#include "generic.h"
#include "noopcart.h"

CartFactory::CartFactory()
{
    // these must be in the same order as umdbase::console_e
    carts[umdbase::UNDEFINED]   = new noopcart();
    carts[umdbase::COLECO] = new GenericCart();
    carts[umdbase::GENESIS]    = new genesis();
    carts[umdbase::SMS]    = new sms();
    carts[umdbase::PCE]    = new GenericCart();
    carts[umdbase::TG16]   = new GenericCart();
}

CartFactory::~CartFactory()
{
    for (int i=0; i < umdbase::CARTS_LEN; i++)
    {
	    delete carts[i];
    }
}

umdbase* CartFactory::getCart(umdbase::console_e mode)
{
    if (mode < umdbase::CARTS_LEN && mode > umdbase::UNDEFINED)
    {
        return carts[mode];
    }
    return carts[umdbase::UNDEFINED];
}

umdbase::console_e CartFactory::getMaxCartMode()
{
    return static_cast<umdbase::console_e>(umdbase::CARTS_LEN - 1);
}

