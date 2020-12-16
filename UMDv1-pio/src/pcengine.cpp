/*******************************************************************//**
 *  \file pcengine.cpp
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

#include "Arduino.h"
#include "umdv1.h"
#include "pcengine.h"

/*******************************************************************//**
 * The constructor
 **********************************************************************/
pcengine::pcengine() 
{
}

/*******************************************************************//**
 * Setup the ports for PC Engine / Turbografx mode
 **********************************************************************/
void pcengine::setup(uint8_t param)
{

    pinMode(nWR, OUTPUT);
    digitalWrite(nWR, HIGH);
    pinMode(nRD, OUTPUT);
    digitalWrite(nRD, HIGH);
    pinMode(nCE, OUTPUT);
    digitalWrite(nRD, HIGH);

    // reset pulse
    pinMode(PCE_nRST, OUTPUT);
    digitalWrite(PCE_nRST, LOW);
    delay(1);
    digitalWrite(PCE_nRST, HIGH);

    info.console = PCE;
    info.mirrored_bus = true;
    info.bus_size = 8;
}

void pcengine::calcChecksum()
{
}

void pcengine::enableSram(uint8_t param)
{
}

void pcengine::disableSram(uint8_t param)
{
}

uint32_t pcengine::getRomSize()
{
    return 0;
}