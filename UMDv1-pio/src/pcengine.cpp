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
#include "umdbase.h"
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
    pinMode(SMS_nRST, OUTPUT);
    digitalWrite(SMS_nRST, LOW);
    delay(1);
    digitalWrite(SMS_nRST, HIGH);

    info.console = static_cast<console_e>(param);
    info.busSize = 8;
}
