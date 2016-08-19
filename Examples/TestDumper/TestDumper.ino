/** \file TestDumper.ino
 *  \author René Richard
 *  \brief This program allows to read and write to various game cartridges including: Genesis, Coleco, SMS, PCE - with possibility for future expansion.
 *  
 *  Target Hardware:
 *  Teensy++2.0 with db Electronics TeensyDumper board rev >= 1.1
 *  Arduino IDE settings:
 *  Board Type  - Teensy++2.0
 *  USB Type    - Serial
 *  CPU Speed   - 16 MHz
 */
 
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

SerialCommand SCmd;
dbDumper db;

/** \brief Main loop
 *  
 *  \return Void
 */
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
    
    //detect cart
    SCmd.addCommand("dt",dbTD_detectCMD);
    SCmd.addCommand("sm",dbTD_setModeCMD);
    SCmd.addCommand("er",dbTD_eraseChipCMD);
    SCmd.addCommand("id",dbTD_flashIDCMD);
    SCmd.addCommand("rw",dbTD_readWordCMD);
    SCmd.addCommand("rb",dbTD_readByteCMD);
    SCmd.addCommand("wb",dbTD_writeByteCMD);
    SCmd.addDefaultHandler(unknownCMD);
}

/** \brief Main loop
 *  
 *  \return Void
 */
void loop()
{
    SCmd.readSerial();
}


/** \brief Prints a list of registered commands on the console
 *  
 *  \param command The unknown command
 *  \return Void
 */
void unknownCMD(const char *command)
{
    Serial.println(F("Unrecognized command: \""));
    Serial.println(command);
    Serial.println(F("\". Registered Commands:"));
    Serial.println(SCmd.getCommandList());  //Returns all registered commands
}


/** \brief Detects whether a cart is connected to the dumper
 *  
 *  If a cart asserts the #CART signal it will be detected.
 *  Note that the mode must be set prior to issuing this command
 *  
 *  Usage:
 *  dt
 *  
 *  \return Void
 */
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


/** \brief Sets the dumper mode
 *  
 *  Configures the dumper's I/O for the corresponding system.
 *  Mode set is required to be issued prior to most other commands as most
 *  commands require a mode to function properly.
 *  
 *  Usage:
 *  sm c
 *  
 *  \return Void
 */
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


/** \brief Erases the contents of the cart
 *  
 *  Erases the correspoding the Flash IC on the cart.
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  er
 *  
 *  \return Void
 */
void dbTD_eraseChipCMD()
{
    db.eraseChip(); 
}


/** \brief Get the Flash ID
 *  
 *  Reads the ID of the corresponding Flash IC.
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  id
 *  id h
 *  
 *  \return Void
 */
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


/** \brief Read a word from the cartridge
 *  
 *  Read a 16bit word from the cartridge, only valid for 16bit buses.
 *  
 *  Usage:
 *  rw 0x0000
 *    - returns unformated word
 *  rw 0x0000 h
 *    - returns HEX formatted word with \n\r
 *  
 *  \return Void
 */
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


/** \brief Read a byte from the cartridge
 *  
 *  Read an 8bit byte from the cartridge. In Coleco mode
 *  the address is forced to uint16_t.
 *  
 *  Usage:
 *  rb 0x0000
 *    - returns unformated byte
 *  rb 0x0000 h
 *    - returns HEX formatted byte with \n\r
 *  
 *  \return Void
 */
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


/** \brief Program a byte in the cartridge
 *  
 *  Program a byte in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  Usage:
 *  wb 0x0000 0x12
 *    - programs 0x12 into address 0x0000
 *  wb 412 12
 *    - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 */
void dbTD_writeByteCMD()
{
    char *arg;
    uint32_t address=0;
    uint8_t data;
    
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    arg = SCmd.next(); 
    data = (uint8_t)strtoul(arg, (char**)0, 0);
    
    db.programByte(address, data, false);
}


/** \brief Program a byte block in the cartridge
 *  
 *  Program a byte block in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  Usage:
 *  wb 0x0000 0x12
 *    - programs 0x12 into address 0x0000
 *  wb 412 12
 *    - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 */
void dbTD_writeBlockCMD()
{
    char *arg;
    uint32_t address = 0;
    uint16_t blockSize = 0, i;
    
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
    
    for( i=0 ; i < blockSize ; i++ )
    {
        db.programByte(address, 0, true);
    }
}



