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

#define WRITE_BLOCK_BUFFER_SIZE     256         //must be a power of 2
#define WRITE_TIMEOUT_MS            10000

SerialCommand SCmd;
dbDumper db;

uint8_t buffer[WRITE_BLOCK_BUFFER_SIZE];

/*******************************************************************//**
 *  \brief Main loop
 *  \return Void
 **********************************************************************/
void setup() {

    uint8_t i;
    
    //hello PC
    Serial.begin(38400);
    //Serial.println(F("db Electronics TeensyDumper v0.2"));

    db.setMode(db.MD);

    //flash to show we're alive
    for( i=0 ; i<4 ; i++ )
    {
        digitalWrite(db.nLED, LOW);
        delay(250);
        digitalWrite(db.nLED, HIGH);
        delay(250);
    }

    //register callbacks for SerialCommand
    SCmd.addCommand("flash",dbTD_flashCMD);
    SCmd.addCommand("detect",dbTD_detectCMD);
    SCmd.addCommand("setmode",dbTD_setModeCMD);
    SCmd.addCommand("erase",dbTD_eraseChipCMD);
    SCmd.addCommand("getid",dbTD_flashIDCMD);
    SCmd.addCommand("readword",dbTD_readWordCMD);
    SCmd.addCommand("readbyte",dbTD_readByteCMD);
    SCmd.addCommand("readbblock",dbTD_readByteBlockCMD);
    SCmd.addCommand("readwblock",dbTD_readWordBlockCMD);
    SCmd.addCommand("progbyte",dbTD_programByteCMD);
    SCmd.addCommand("progbblock",dbTD_programByteBlockCMD);
    SCmd.addDefaultHandler(unknownCMD);
    
    SCmd.clearBuffer();
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
 *  \brief Auto responce for detection on PC side
 *  
 *  Usage:
 *  flash
 *  
 *  \return Void
 **********************************************************************/
void dbTD_flashCMD()
{
    Serial.println(F("thunder"));
    digitalWrite(db.nLED, LOW);
	delay(100);
	digitalWrite(db.nLED, HIGH);
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
            db.setMode(db.MD);
            Serial.println(F("mode = g")); 
            break;
        case 'p':
            db.setMode(db.TG);
            Serial.println(F("mode = p")); 
            break;
        case 'c':
            db.setMode(db.CV);
            Serial.println(F("mode = c")); 
            break;
        default:
            Serial.println(F("mode = undefined")); 
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
    
    //read the flash ID
    data = db.getFlashID();

	//check if we should output a formatted string
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
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //read the word
    data = db.readWord(address);

	//check if we should output a formatted string
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
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    if( db.getMode() == db.CV )
    {
		address = db.convColecoAddr(address);
	}
    
	data = db.readByte(address);

	//check if we should output a formatted string
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
 *  \brief Read a block of bytes from the cartridge
 *  Read an 8bit byte from the cartridge. In Coleco mode
 *  the address is forced to uint16_t.
 *  
 *  Usage:
 *  readbbyte 0x0000 128
 *    - returns 128 unformated bytes
 *  
 *  \return Void
 **********************************************************************/
void dbTD_readByteBlockCMD()
{
    char *arg;
    uint32_t address = 0;
    uint16_t blockSize = 0, i;
    uint8_t data;

    //get the address in the next argument
	arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
    
    //Serial.print(F("address "));
	//Serial.print(address, HEX);
	//Serial.print(F(" size " ));
	//Serial.println(blockSize, HEX);
    
    //arg = SCmd.next();
    //if( arg != NULL )
    //{
		//Serial.print(F("address "));
		//Serial.print(address, HEX);
		//Serial.print(F(" size " ));
		//Serial.println(blockSize, HEX);
	//}
	
    for( i = 0; i < blockSize; i++ )
    {
		data = db.readByte(address++);
		Serial.print(F("address "));
		Serial.print(address, HEX);
		Serial.print(F(" data " ));
		Serial.println(data, HEX);
		//Serial.write((char)(data));
	}
}

/*******************************************************************//**
 *  \brief Read a block of words from the cartridge
 *  
 *  Usage:
 *  readbbyte 0x0000 128
 *    - returns 128 unformated bytes
 *  
 *  \return Void
 **********************************************************************/
void dbTD_readWordBlockCMD()
{
    char *arg;
    uint32_t address = 0;
    uint32_t blockSize = 0, i;
    uint16_t data;

    //get the address in the next argument
	arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
	
    for( i = 0; i < blockSize; i += 2 )
    {
		data = db.readWord(address);
		address += 2;
		Serial.write((char)(data));
        Serial.write((char)(data>>8));
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
    uint8_t data, readBack;
        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the data in the next argument
    arg = SCmd.next(); 
    data = (uint8_t)strtoul(arg, (char**)0, 0);
	readBack = ~data;

    //if coleco, force 16 bit address program
    if( db.getMode() == db.CV )
    {
		address = db.convColecoAddr(address);      
    }
    
    db.programByte(address, data, false);
    
    //check if we should verify the write
    arg = SCmd.next();
	if( arg != NULL )
    {
        switch(*arg)
        {
			case 'v':
				delayMicroseconds(50);
				readBack = db.readByte(address);
				break;
			default:
				break;
        }
        
        //compare values
        if( readBack == data )
        {
			//check if we should output a formatted string
			arg = SCmd.next();
			if( arg != NULL )
			{
				switch(*arg)
				{
					case 'h':
						Serial.print(F("ok 0x")); 
						Serial.print(readBack,HEX);
						Serial.print(F(" at address 0x")); 
						Serial.println(address,HEX);
						break;
					default:
						break;
				}
			}else
			{
				Serial.write((char)(data));
			}
		}else
		{
			//check if we should output a formatted string
			arg = SCmd.next();
			if( arg != NULL )
			{
				switch(*arg)
				{
					case 'h':
						Serial.print(F("read 0x")); 
						Serial.print(readBack,HEX);
						Serial.print(F(" expected 0x")); 
						Serial.print(data,HEX);
						Serial.print(F(" at address 0x")); 
						Serial.println(address,HEX);
						break;
					default:
						break;
				}
			}else
			{
				Serial.write((char)(data));
			}
		}
    }
    
    
}


/*******************************************************************//**
 * \brief Program a byte block in the cartridge
 * Program a byte block in the cartridge. Prior to progamming,
 * the sector or entire chip must be erased. The function uses data
 * polling between each byte program to validate the operation. If no 
 * data is received for WRITE_TIMEOUT_MS the function will abort.
 *  
 * Usage:
 * progbblock 0x0000 64 %64bytes%
 *   - programs %64bytes% received starting at address 0x0000
 *  
 * \return Void
 **********************************************************************/
void dbTD_programByteBlockCMD()
{
    char *arg;
    uint32_t address = 0;
    uint16_t blockSize = 0, i, j;
    uint8_t count = 0;
    uint32_t timeout = millis();
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
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
                buffer[count++] = Serial.read();
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
            if( db.getMode() == db.CV )
            {
                db.programByte((uint16_t)address++, buffer[j], true);
            }else
            {
                db.programByte(address++, buffer[j], true);
            }
        }
        
        count = 0;
        Serial.println(i,HEX);
    }
    
    SCmd.clearBuffer();
}



