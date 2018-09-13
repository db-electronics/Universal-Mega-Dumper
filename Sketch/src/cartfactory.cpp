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
#include "carts/genesis.h"
#include "carts/sms.h"
#include "carts/generic.h"
#include "carts/noopcart.h"

CartFactory::CartFactory()
{
    carts[CartFactory::UNDEFINED]   = new noopcart();
    carts[CartFactory::COLECO] = new GenericCart();
    carts[CartFactory::GEN]    = new genesis();
    carts[CartFactory::SMS]    = new sms();
    carts[CartFactory::PCE]    = new GenericCart();
    carts[CartFactory::TG16]   = new GenericCart();
    carts[CartFactory::SNES]   = new GenericCart();
    carts[CartFactory::SNESLO] = new GenericCart();
}

CartFactory::~CartFactory()
{
    for (int i=0; i < CARTS_LEN; i++)
    {
	delete carts[i];
    }
}

umdbase* CartFactory::getCart(CartFactory::Mode mode)
{
    if (mode < CARTS_LEN && mode > CartFactory::UNDEFINED)
    {
        return carts[mode];
    }
    return carts[CartFactory::UNDEFINED];
}

CartFactory::Mode CartFactory::getMaxCartMode()
{
    return static_cast<Mode>(CARTS_LEN);
}

