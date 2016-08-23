/*******************************************************************//**
 *  \file TestDumper.ino
 *  \author René Richard
 *  \brief This program allows to read and write to various game cartridges including: Genesis, Coleco, SMS, PCE - with possibility for future expansion.
 *  
 *  Target Hardware:
 *  Teensy++2.0 with db Electronics TeensyDumper board rev >= 1.1
 *  Arduino IDE settings:
 *  Board Type  - Teensy++2.0
 *  USB Type    - Serial
 *  CPU Speed   - 16 MHz
 **********************************************************************/
 
/*
 LICENSE
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SoftwareSerial.h>
#include <SerialCommand.h>
#include <dbDumper.h>

#define WRITE_BLOCK_BUFFER_SIZE     128         //must be a power of 2
#define WRITE_TIMEOUT_MS            10000

SerialCommand SCmd;
dbDumper db;

/*******************************************************************//**
 *  \brief Main loop
 *  \return Void
 **********************************************************************/
void setup() {

    uint8_t i;

    //hello PC
    Serial.begin(38400);
    Serial.println(F("db Electronics TeensyDumper v0.2"));

    db.setMode(db.genesis);

    //flash to show we're alive
    for( i=0 ; i<4 ; i++ )
    {
        digitalWrite(db.nLED, LOW);
        delay(250);
        digitalWrite(db.nLED, HIGH);
        delay(250);
    }

    //register callbacks for SerialCommand
    SCmd.addCommand("detect",dbTD_detectCMD);
    SCmd.addCommand("setmode",dbTD_setModeCMD);
    SCmd.addCommand("erase",dbTD_eraseChipCMD);
    SCmd.addCommand("getid",dbTD_flashIDCMD);
    SCmd.addCommand("readword",dbTD_readWordCMD);
    SCmd.addCommand("readbyte",dbTD_readByteCMD);
    SCmd.addCommand("progbyte",dbTD_programByteCMD);
    SCmd.addCommand("progbblock",dbTD_programByteBlockCMD);
    SCmd.addDefaultHandler(unknownCMD);
}

/*******************************************************************//**
 *  \brief Main loop
 *  \return Void
 **********************************************************************/
void loop()
{
    SCmd.readSerial();
}


/*******************************************************************//**
 *  \brief Prints a list of registered commands on the console
 *  \param command The unknown command
 *  \return Void
 **********************************************************************/
void unknownCMD(const char *command)
{
    Serial.println(F("Unrecognized command: \""));
    Serial.println(command);
    Serial.println(F("\". Registered Commands:"));
    Serial.println(SCmd.getCommandList());  //Returns all registered commands
}


/*******************************************************************//**
 *  \brief Detects whether a cart is connected to the dumper
 *  If a cart asserts the #CART signal it will be detected.
 *  Note that the mode must be set prior to issuing this command
 *  
 *  Usage:
 *  detect
 *  
 *  \return Void
 **********************************************************************/
void dbTD_detectCMD()
{
    if(db.detectCart())
    {
        Serial.println(F("True")); 
    }else
    {
        Serial.println(F("False")); 
    }
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
void dbTD_setModeCMD()
{
    char *arg;
    arg = SCmd.next();
    switch(*arg)
    {
        case 'g':
            db.setMode(db.genesis);
            Serial.println(F("mode set genesis")); 
            break;
        case 'p':
            db.setMode(db.pcengine);
            Serial.println(F("mode set genesis")); 
            break;
        case 'c':
            db.setMode(db.coleco);
            Serial.println(F("mode set coleco")); 
            break;
        default:
            Serial.println(F("mode set undefined")); 
            db.setMode(db.undefined);
            break;
    }  
}


/*******************************************************************//**
 *  \brief Erases the contents of the cart
 *  Erases the correspoding the Flash IC on the cart.
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  erase
 *  erase w
 *  
 *  \return Void
 **********************************************************************/
void dbTD_eraseChipCMD()
{
    char *arg;
    uint32_t eraseTime;

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            case 'w':
                eraseTime = db.eraseChip(true);
                Serial.print(F("erase executed in ")); 
                Serial.print(eraseTime,DEC);
                Serial.println(F("ms"));
                break;
            default:
                break;
        }
    }else
    {
        db.eraseChip(false); 
    }
}


/*******************************************************************//**
 *  \brief Get the Flash ID
 *  Reads the ID of the corresponding Flash IC.
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  getid
 *  getid h
 *  
 *  \return Void
 **********************************************************************/
void dbTD_flashIDCMD()
{
    char *arg;
    uint16_t data;
    data = db.getFlashID();

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            case 'h':
                Serial.println(data,HEX);
                break;
            default:
                break;
        }
    }else
    {
        Serial.write((char)(data));
        Serial.write((char)(data>>8));
    }
}


/*******************************************************************//**
 *  \brief Read a word from the cartridge
 *  Read a 16bit word from the cartridge, only valid for 16bit buses.
 *  
 *  Usage:
 *  readword 0x0000
 *    - returns unformated word
 *  readword 0x0000 h
 *    - returns HEX formatted word with \n\r
 *  
 *  \return Void
 **********************************************************************/
void dbTD_readWordCMD()
{
    char *arg;
    uint32_t address=0;
    uint16_t data;
    arg = SCmd.next();
    
    address = strtoul(arg, (char**)0, 0);
    data = db.readWord(address);

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            case 'h':
                Serial.println(data,HEX);
                break;
            default:
                break;
        }   
    }else
    {
        Serial.write((char)(data));
        Serial.write((char)(data>>8));
    }

}


/*******************************************************************//**
 *  \brief Read a byte from the cartridge
 *  Read an 8bit byte from the cartridge. In Coleco mode
 *  the address is forced to uint16_t.
 *  
 *  Usage:
 *  readbyte 0x0000
 *    - returns unformated byte
 *  readbyte 0x0000 h
 *    - returns HEX formatted byte with \n\r
 *  
 *  \return Void
 **********************************************************************/
void dbTD_readByteCMD()
{
    char *arg;
    uint32_t address = 0;
    uint8_t data;
    arg = SCmd.next();
    
    address = strtoul(arg, (char**)0, 0);
    
    //if coleco, force 16 bit address
    if( db.getMode() == db.coleco )
    {
        data = db.readByte((uint16_t)address);
    }else
    {
        data = db.readByte(address);
    }

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
        case 'h':
            Serial.println(data,HEX);
            break;
        default:
            break;
        }
    }else
    {
        Serial.write((char)(data));
    }
}


/*******************************************************************//**
 *  \brief Program a byte in the cartridge
 *  Program a byte in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  Usage:
 *  progbyte 0x0000 0x12
 *    - programs 0x12 into address 0x0000
 *  progbyte 412 12
 *    - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 **********************************************************************/
void dbTD_programByteCMD()
{
    char *arg;
    uint32_t address=0;
    uint8_t data;
    
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    arg = SCmd.next(); 
    data = (uint8_t)strtoul(arg, (char**)0, 0);

    //if coleco, force 16 bit address
    if( db.getMode() == db.coleco )
    {
        db.programByte((uint16_t)address, data, false);
    }else
    {
        db.programByte(address, data, false);
    }
}


/*******************************************************************//**
 *  \brief Program a byte block in the cartridge
 *  Program a byte block in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  Usage:
 *  progbblock 0x0000 0x12
 *    - programs 0x12 into address 0x0000
 *  progbblock 412 12
 *    - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 **********************************************************************/
void dbTD_programByteBlockCMD()
{
    char *arg;
    uint32_t address = 0;
    uint16_t blockSize = 0, i, j;

    uint8_t buf[WRITE_BLOCK_BUFFER_SIZE];
    uint8_t count = 0;

    uint32_t timeout = millis();
    
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);

    //write bytes in chunks of WRITE_BLOCK_BUFFER_SIZE bytes 
    for( i = 0 ; i < blockSize ; i += WRITE_BLOCK_BUFFER_SIZE )
    {
        //receive a block in buffer before writing to flash
        while( count < WRITE_BLOCK_BUFFER_SIZE )
        {
            if(Serial.available())
            {
                buf[count++] = Serial.read();
                timeout = millis() + WRITE_TIMEOUT_MS;
            }else
            {
                if( millis() > timeout )
                {
                    Serial.println(F("read timeout"));
                    SCmd.clearBuffer();
                    return;
                }
            }
        }

        //write the block in buffer to flash
        for( j = 0 ; j < WRITE_BLOCK_BUFFER_SIZE ; j++)
        {
            //if coleco, force 16 bit address
            if( db.getMode() == db.coleco )
            {
                db.programByte((uint16_t)address++, buf[j], true);
            }else
            {
                db.programByte(address++, buf[j], true);
            }
        }
        
        count = 0;
        Serial.println(i,HEX);
    }
    
    SCmd.clearBuffer();
}



