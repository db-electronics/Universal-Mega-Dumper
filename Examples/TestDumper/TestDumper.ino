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
#include <SerialCommand.h>			// https://github.com/PaulStoffregen/SerialFlash
#include <SerialFlash.h>			// https://github.com/PaulStoffregen/SerialFlash
#include <SPI.h>
#include <dbDumper.h>

#define WRITE_BLOCK_BUFFER_SIZE     256         //must be a power of 2
#define WRITE_TIMEOUT_MS            10000

#define FLASH_BUFFER_SIZE    		512

SerialCommand SCmd;
dbDumper db;

const int FlashChipSelect = 20; 	// digital pin for flash chip CS pin
SerialFlashFile flashFile;
uint8_t sflid[5];
uint32_t sflSize;

union{
	char 		byte[1024];
	uint16_t 	word[512];
} writeBuffer;

char buffer[2]; //TODO get rid of this buffer, use above union instead

/*******************************************************************//**
 *  \brief Main loop
 *  \return Void
 **********************************************************************/
void setup() {

    uint8_t i;
    
    //hello PC
    Serial.begin(115200);
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

	if (!SerialFlash.begin(FlashChipSelect)) {
		//error("Unable to access SPI Flash chip");
	}else
	{
		SerialFlash.readID(sflid);
	}

    //register callbacks for SerialCommand related to the cartridge
    SCmd.addCommand("flash",dbTD_flashCMD);
    SCmd.addCommand("detect",dbTD_detectCMD);
    SCmd.addCommand("setmode",dbTD_setModeCMD);
    SCmd.addCommand("erase",dbTD_eraseChipCMD);
    SCmd.addCommand("getid",dbTD_flashIDCMD);
    SCmd.addCommand("readword",dbTD_readWordCMD);
    SCmd.addCommand("readbyte",dbTD_readByteCMD);
    SCmd.addCommand("readbblock",dbTD_readByteBlockCMD);
    SCmd.addCommand("readwblock",dbTD_readWordBlockCMD);
    SCmd.addCommand("writebyte",dbTD_writeByteCMD);
    SCmd.addCommand("writeword",dbTD_writeWordCMD);
    SCmd.addCommand("progbyte",dbTD_programByteCMD);
    SCmd.addCommand("progbblock",dbTD_programByteBlockCMD);
    SCmd.addCommand("progword",dbTD_programWordCMD);
    SCmd.addCommand("progwblock",dbTD_programWordBlockCMD);
    
    //register callbacks for SerialCommand related to the onboard serial flash
    SCmd.addCommand("sflgetid",dbTD_sflIDCMD);
    SCmd.addCommand("sflerase",dbTD_sflErase);
    SCmd.addCommand("sflwrite",dbTD_sflWriteFile);
    
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
 *  \brief Read the onboard serial flash ID
 *  
 *  Usage:
 *  sflgetid
 *  
 *  \return Void
 **********************************************************************/
void dbTD_sflIDCMD()
{
	//W25Q128FVSIG
    Serial.write(sflid[0]);
    Serial.write(sflid[1]);
    Serial.write(sflid[2]);
    Serial.write(sflid[3]);
    Serial.write(sflid[4]);
}

/*******************************************************************//**
 *  \brief Erase the serial flash
 *  
 *  Usage:
 *  sflerase
 *  
 *  \return Void
 **********************************************************************/
void dbTD_sflErase()
{
	char *arg;

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
			//wait for operation to complete, measure time
            case 'w':
                SerialFlash.eraseAll();
				while (SerialFlash.ready() == false) {
					// wait, 30 seconds to 2 minutes for most chips
					delay(500);
					Serial.print(".");
				}
				Serial.print("!");
                break;
            default:
                break;
        }
    }else
    {
        SerialFlash.eraseAll();
    }
}

/*******************************************************************//**
 *  \brief Write a file to the serial flash
 *  
 *  Usage:
 *  sflwrite rom.bin 4096 xx[0] xx[1] xx[2] ... xx[4095] 
 *  
 *  \return Void
 **********************************************************************/
void dbTD_sflWriteFile()
{
	char *arg;
	uint16_t i, count;
	uint32_t fileSize, pos=0;
	char fileName[13]; 		//Max filename length (8.3 plus a null char terminator)

	//get the file name
    arg = SCmd.next();
    i = 0;
    while( (*arg != 0) || (i < 13) )
    {
		fileName[i++] = *(arg++);
	}
	fileName[i] = 0; //null char terminator

	//test filename capture
	i = 0;
	Serial.print(F("filename = '"));
	while( fileName[i] != 0 )
	{
		Serial.write(fileName[i++]);
	}
    Serial.println(F("'"));

	//get the size in the next argument
    arg = SCmd.next();
    fileSize = strtoul(arg, (char**)0, 0);

	//Serial.read(); //there's an extra byte here for some reason - discard

	//SerialFlash.create(fileName, fileSize);
	//flashFile = SerialFlash.open(fileName);
	//while( pos < fileSize )
	//{
		//// fill buffer from USB
		//count = 0;
		//while( count < FLASH_BUFFER_SIZE )
		//{
			//if( Serial.available() )
			//{
				//writeBuffer.byte[count++] = Serial.read();
			//}
		//}
		//// write buffer to serial flash file
		//flashFile.write(writeBuffer.byte, FLASH_BUFFER_SIZE);
		//Serial.println(F("rdy"));
	//}
    
    Serial.print(F("size = "));
    Serial.println(fileSize, HEX);
}

/*******************************************************************//**
 *  \brief Auto response for detection on PC side
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
 *  \brief Detects the state of the #CART signal
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
            db.setMode(db.PC);
            Serial.println(F("mode = p")); 
            break;
        case 't':
            db.setMode(db.TG);
            Serial.println(F("mode = t")); 
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
	uint8_t chip;

	//get the chip number in the next argument
    arg = SCmd.next();
    chip = (uint8_t)strtoul(arg, (char**)0, 0);

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
			//wait for operation to complete, measure time
            case 'w':
                db.eraseChip(true, chip);
                break;
            default:
                break;
        }
    }else
    {
        db.eraseChip(false, chip);
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
    
    uint32_t data;
    
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
        Serial.write((char)(data>>16));
        Serial.write((char)(data>>24));
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
    
	data = db.readByte(address, true);

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
	
    for( i = 0; i < blockSize; i++ )
    {
		data = db.readByte(address++, true);
		Serial.write((char)(data));
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
	
	//read words from block, output converts to little endian
    for( i = 0; i < blockSize; i += 2 )
    {
		data = db.readWord(address);
		address += 2;
		Serial.write((char)(data));
        Serial.write((char)(data>>8));
	}
}

/*******************************************************************//**
 *  \brief Write a byte to the cartridge
 *  Not to confuse with programming, write does not attempt to modify
 *  the flash array.
 *  
 *  Usage:
 *  writebyte 0x100 0xFE
 *    - writes 0xFE to address 0x100
 *  
 *  \return Void
 **********************************************************************/
void dbTD_writeByteCMD()
{
    char *arg;
    uint32_t address = 0;
    uint8_t data;

    //get the address in the next argument
	arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the word in the next argument
    arg = SCmd.next(); 
    data = strtoul(arg, (char**)0, 0);
	
	//write word
	db.writeByte(address, data);
}

/*******************************************************************//**
 *  \brief Write a word to the cartridge
 *  Not to confuse with programming, write does not attempt to modify
 *  the flash array.
 *  
 *  Usage:
 *  writeword 0x100 0xFE5A
 *    - writes 0xFE5A to address 0x100
 *  
 *  \return Void
 **********************************************************************/
void dbTD_writeWordCMD()
{
    char *arg;
    uint32_t address = 0;
    uint16_t data;

    //get the address in the next argument
	arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    address &= 0xFFFFFFFE;
    
    //get the word in the next argument
    arg = SCmd.next(); 
    data = strtoul(arg, (char**)0, 0);
	
	//write word
	db.writeWord(address, data);
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
		address = (uint16_t)(address);      
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
				readBack = db.readByte(address, true);
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
    uint32_t address=0;
    uint16_t size;
    uint8_t count=0;
	        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    size = strtoul(arg, (char**)0, 0);
    
    //receive size bytes
    Serial.read(); //there's an extra byte here for some reason - discard
    
    while( count < size )
    {
		if( Serial.available() )
		{
			writeBuffer.byte[count++] = Serial.read();
		}
	}
	
	SCmd.clearBuffer();
	
	//program size bytes
	count = 0;
	while( count < size )
	{
		db.programByte(address, writeBuffer.byte[count++], true);
		address++;
	}
	
	Serial.println(F("done"));
}

/*******************************************************************//**
 *  \brief Program a word in the cartridge
 *  Program a word in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  Usage:
 *  progword 0x0000 0x12
 *    - programs 0x12 into address 0x0000
 *  progword 412 12
 *    - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 **********************************************************************/
void dbTD_programWordCMD()
{
    char *arg;
    uint32_t address=0;
    uint16_t data;
        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the data in the next argument
    arg = SCmd.next(); 
    data = (uint16_t)strtoul(arg, (char**)0, 0);
    
    db.programWord(address, data, false);
}

/*******************************************************************//**
 *  \brief Program a word block in the cartridge
 *  Program a word in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  Usage:
 *  progword 0x0000 0x12
 *    - programs 0x12 into address 0x0000
 *  progword 412 12
 *    - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 **********************************************************************/
void dbTD_programWordBlockCMD()
{
    char *arg;
    uint32_t address=0;
    uint16_t size;
    uint8_t count=0;
	        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    size = strtoul(arg, (char**)0, 0);
    
    //receive size bytes
    Serial.read(); //there's an extra byte here for some reason - discard
    
    while( count < size )
    {
		if( Serial.available() )
		{
			writeBuffer.byte[count++] = Serial.read();
		}
	}
	
	SCmd.clearBuffer();
	
	//program size/2 words
	count = 0;
	while( count < ( size >> 1) )
	{
		db.programWord(address, writeBuffer.word[count++], true);
		address += 2;
	}
	
	Serial.println(F("done"));
}

