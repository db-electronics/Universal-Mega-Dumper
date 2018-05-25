/*******************************************************************//**
 *  \file Interface.ino
 *  \author René Richard
 *  \brief This program provides a serial interface over USB to the
 *         Universal Mega Dumper. 
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

#include <SoftwareSerial.h>         
#include <SerialCommand.h>                  // https://github.com/db-electronics/ArduinoSerialCommand
#include <SerialFlash.h>                    // https://github.com/PaulStoffregen/SerialFlash
#include <SPI.h>
#include <umdbase.h>
#include <genesis.h>

#define DATA_BUFFER_SIZE            2048    ///< Size of serial receive data buffer

SerialCommand SCmd;                         ///< Receive and parse serial commands
umdbase *cart;                              ///< Pointer to all cartridge classes
genesis genCart;                            ///< Genesis cartridge type

const int FlashChipSelect = 20;             ///< Digital pin for flash chip CS pin
SerialFlashFile flashFile;                  ///< Serial flash file object
uint8_t sfID[5];                            ///< Serial flash file id
uint32_t sfSize;                            ///< Serial flash file size

union dataBuffer{
    char        byte[DATA_BUFFER_SIZE];     ///< byte access within dataBuffer
    uint16_t    word[DATA_BUFFER_SIZE/2];   ///< word access within dataBuffer
} dataBuffer;                               ///< union of byte/words to permit the Rx of bytes and Tx of words without hassle

/*******************************************************************//**
 *  \brief Flash the LED, initialize the serial flash memory
 *         and register all serial commands.
 *  \return Void
 **********************************************************************/
void setup() {

    uint8_t i;
    
    Serial.begin(460800);

    //flash to show we're alive
    for( i=0 ; i<2 ; i++ )
    {
        digitalWrite(genCart.nLED, LOW);
        delay(100);
        digitalWrite(genCart.nLED, HIGH);
        delay(100);
    }

    if (!SerialFlash.begin(FlashChipSelect)) {
        //error("Unable to access SPI Flash chip");
    }else
    {
        SerialFlash.readID(sfID);
        sfSize = SerialFlash.capacity(sfID);
    }

    //register callbacks for SerialCommand related to the cartridge
    SCmd.addCommand("flash",  _flashThunder);
    SCmd.addCommand("setmode",_setMode);
    
    //register callbacks for SerialCommand related to the cartridge
    SCmd.addCommand("getid",  getFlashID);
    
    SCmd.addDefaultHandler(_unknownCMD);
    SCmd.clearBuffer();
}

/*******************************************************************//**
 *  \brief Keep checking for serial commands.
 *  \return Void
 **********************************************************************/
void loop()
{
    SCmd.readSerial();
}

/*******************************************************************//**
 *  \brief Prints a list of registered commands
 *  \param command The unknown command
 *  \return Void
 **********************************************************************/
void _unknownCMD(const char *command)
{
    Serial.println(F("Unrecognized command: \""));
    Serial.println(command);
    Serial.println(F("\". Registered Commands:"));
    Serial.println(SCmd.getCommandList());
}

/*******************************************************************//**
 *  \brief Auto response for detection on PC side
 *  \return Void
 **********************************************************************/
void _flashThunder()
{
    Serial.println(F("thunder"));
}

/*******************************************************************//**
 *  \brief Sets the dumper mode
 *  Configures the dumper's I/O for the corresponding system.
 *  Mode set is required to be issued prior to most other commands as most
 *  commands require a mode to function properly.
 *  
 *  Usage:
 *  setmode c
 *  
 *  \return Void
 **********************************************************************/
void _setMode()
{
    char *arg;
    uint8_t mode;
    
    arg = SCmd.next();
    mode = (uint8_t)strtoul(arg, (char**)0, 0);
    
    switch(mode)
    {
        //Colecovision
        case 1:
            
            Serial.println(F("mode = 1")); 
            break;
        //Genesis
        case 2:
            cart = &genCart;
            cart->setup();
            Serial.println(F("mode = 2")); 
            break;
        //Master System
        case 3:
            
            Serial.println(F("mode = 3")); 
            break;
        //PC Engine
        case 4:
            
            Serial.println(F("mode = 4")); 
            break;
        //Turbografx-16
        case 5:
            
            Serial.println(F("mode = 5"));
            break;
        //SNES    
        case 6:
            
            Serial.println(F("mode = 6"));
            break;
        //SNESLO
        case 7:
        
            Serial.println(F("mode = 6"));
            break;
        default:
            Serial.println(F("mode = undefined")); 
            break;
    }  
}

/*******************************************************************//**
 *  \brief Get the Flash ID
 *  Reads the ID of the corresponding Flash IC.
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  getid
 *  
 *  \return Void
 **********************************************************************/
void getFlashID()
{
    char *arg;

    //check for next argument, if present
    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            //manufacturer
            case 'm':
                Serial.write((char)(cart->flashID.manufacturer));
                break;
            //device
            case 'd':
                Serial.write((char)(cart->flashID.device));
                break;
            //boot mode
            case 't':
                Serial.write((char)(cart->flashID.type));
                break;
            //size
            case 's':
                Serial.write((char)(cart->flashID.size));
                Serial.write((char)(cart->flashID.size>>8));
                Serial.write((char)(cart->flashID.size>>16));
                Serial.write((char)(cart->flashID.size>>24));
                break;
            default:
                break;
        }
    }else
    {
        // query the chip when no paramters are passed
        cart->getFlashID(0);
    }
            
}

