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
#include "umdv1.h"
#include "cartfactory.h"
#include "genesis.h"
#include "sms.h"
#include "pcengine.h"
#include "turbografx.h"
#include "generic.h"
#include "noopcart.h"

CartFactory::CartFactory()
{
    // these must be in the same order as umdv1::console_e
    carts[umdv1::UNDEFINED]   = new noopcart();
    carts[umdv1::GENESIS]    = new genesis();
    carts[umdv1::SMS]    = new sms();
    carts[umdv1::PCE]    = new pcengine();
    carts[umdv1::TG16]   = new turbografx();
}

CartFactory::~CartFactory()
{
    for (int i=0; i < umdv1::CARTS_LEN; i++)
    {
	    delete carts[i];
    }
}

umdv1* CartFactory::getCart(umdv1::console_e mode)
{
    if (mode < umdv1::CARTS_LEN && mode > umdv1::UNDEFINED)
    {
        return carts[mode];
    }
    return carts[umdv1::UNDEFINED];
}

umdv1::console_e CartFactory::getMaxCartMode()
{
    return static_cast<umdv1::console_e>(umdv1::CARTS_LEN - 1);
}

