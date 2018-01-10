/*******************************************************************//**
 *  \file Interface.ino
 *  \author René Richard
 *  \brief This program provides a serial interface over USB to the
 *         Universal Mega Dumper. 
 *
 * LICENSE
 *
 *   This file is part of Universal Mega Dumper.
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
#include <SerialCommand.h>					// https://github.com/db-electronics/ArduinoSerialCommand
#include <SerialFlash.h>					// https://github.com/PaulStoffregen/SerialFlash
#include <SPI.h>
#include <umd.h>

#define DATA_BUFFER_SIZE			2048	///< Size of serial receive data buffer

SerialCommand SCmd;							///< Receive and parse serial commands
umd umd;									///< Universal Mega Dumper declaration

const int FlashChipSelect = 20; 			///< Digital pin for flash chip CS pin
SerialFlashFile flashFile;					///< Serial flash file object
uint8_t sfID[5];							///< Serial flash file id
uint32_t sfSize;							///< Serial flash file size

union dataBuffer{
	char 		byte[DATA_BUFFER_SIZE];		///< byte access within dataBuffer
	uint16_t 	word[DATA_BUFFER_SIZE/2];	///< word access within dataBuffer
} dataBuffer;								///< union of byte/words to permit the Rx of bytes and Tx of words without hassle

/*******************************************************************//**
 *  \brief Flash the LED, initialize the serial flash memory
 *         and register all serial commands.
 *  \return Void
 **********************************************************************/
void setup() {

    uint8_t i;
    
    Serial.begin(115200);

    umd.setMode(umd.MD);

    //flash to show we're alive
    for( i=0 ; i<4 ; i++ )
    {
        digitalWrite(umd.nLED, LOW);
        delay(250);
        digitalWrite(umd.nLED, HIGH);
        delay(250);
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
    SCmd.addCommand("detect", _detect);
    SCmd.addCommand("setmode",_setMode);
    
    //register callbacks for SerialCommand related to the cartridge
    SCmd.addCommand("erase",  eraseChip);
    SCmd.addCommand("getid",  getFlashID);
    
    SCmd.addCommand("rdbyte", readByte);
    SCmd.addCommand("rdbblk", readByteBlock);
    SCmd.addCommand("rdsbblk",readSRAMByteBlock);
    SCmd.addCommand("rdword", readWord);
    SCmd.addCommand("rdwblk", readWordBlock);
    SCmd.addCommand("rdswblk",readSRAMWordBlock);
    
    SCmd.addCommand("wrbyte", writeByte);
    SCmd.addCommand("wrsblk", writeSRAMByteBlock);
    SCmd.addCommand("wrbrblk",writeBRAMByteBlock);
    SCmd.addCommand("wrword", writeWord);
    
    SCmd.addCommand("prgbyte",programByte);
    SCmd.addCommand("prgbblk",programByteBlock);
    SCmd.addCommand("prgword",programWord);
    SCmd.addCommand("prgwblk",programWordBlock);
    
    //register callbacks for SerialCommand related to the onboard serial flash
    SCmd.addCommand("sfgetid",sfGetID);
    SCmd.addCommand("sfsize", sfGetSize);
    SCmd.addCommand("sferase",sfEraseAll);
    SCmd.addCommand("sfwrite",sfWriteFile);
    SCmd.addCommand("sflist", sfListFiles);
    SCmd.addCommand("sfread", sfReadFile);
    SCmd.addCommand("sfburn", sfBurnCart);
    
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
    digitalWrite(umd.nLED, LOW);
	delay(100);
	digitalWrite(umd.nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Read the onboard serial flash ID
 *  \return Void
 **********************************************************************/
void sfGetID()
{
	//W25Q128FVSIG
    Serial.write(sfID[0]);
    Serial.write(sfID[1]);
    Serial.write(sfID[2]);
    Serial.write(sfID[3]);
    Serial.write(sfID[4]);
}

/*******************************************************************//**
 *  \brief Get the size of the onboard serial flash
 *  \return Void
 **********************************************************************/
void sfGetSize()
{
	//W25Q128FVSIG
    Serial.println(sfSize,DEC);
}

/*******************************************************************//**
 *  \brief Erase the serial flash
 *  \return Void
 **********************************************************************/
void sfEraseAll()
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
					digitalWrite(umd.nLED, LOW);
					delay(250);
					digitalWrite(umd.nLED, HIGH);
					delay(250);
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
 *  \brief Burn a file from the serial flash to the cartridge
 *  
 *  Usage:
 *  sflburn rom.bin
 *  
 *  \return Void
 **********************************************************************/
void sfBurnCart()
{
	char *arg;
	uint16_t blockSize, i;
	uint32_t fileSize, address=0, pos=0;
	char fileName[13]; 		//Max filename length (8.3 plus a null char terminator)
	
	//get the file name
    arg = SCmd.next();
    i = 0;
    while( (*arg != 0) && ( i < 12) )
    {
		fileName[i++] = *(arg++);
	}
	fileName[i] = 0; //null char terminator
	
		//get the read block size
	arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
	flashFile = SerialFlash.open(fileName);
	if (flashFile)
	{
		Serial.println(F("found"));
		fileSize = flashFile.size();
		Serial.println(fileSize,DEC);
		
		while( pos < fileSize )
		{
			flashFile.read(dataBuffer.byte, blockSize);
			
			switch( umd.getMode() )
			{
				case umd.MD:
					for( i=0 ; i < ( blockSize >> 1) ; i++ )
					{
						umd.programWord(address, dataBuffer.word[i], true);
						address += 2;
					}
					break;
				default:
					for( i=0 ; i < blockSize ; i++ )
					{
						umd.programByte(address, dataBuffer.byte[i], true);
						address++;
					}
					break;
			}
			pos += blockSize;
			Serial.println(pos, DEC);
		}
		Serial.println(F("done"));
	}else
	{
		Serial.println(F("error"));
	}
	
	flashFile.close();
}

/*******************************************************************//**
 *  \brief Read a file from the serial flash
 *  
 *  Usage:
 *  sflread file.bin
 *  
 *  \return Void
 **********************************************************************/
void sfReadFile()
{
	char *arg;
	uint16_t blockSize, i;
	uint32_t fileSize, pos=0;
	char fileName[13]; 		//Max filename length (8.3 plus a null char terminator)
	
	//get the file name
    arg = SCmd.next();
    i = 0;
    while( (*arg != 0) && ( i < 12) )
    {
		fileName[i++] = *(arg++);
	}
	fileName[i] = 0; //null char terminator
	
	//get the read block size
	arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
	flashFile = SerialFlash.open(fileName);
	if (flashFile)
	{
		Serial.println(F("found"));
		fileSize = flashFile.size();
		Serial.println(fileSize,DEC);
		
		while( pos < fileSize )
		{
			flashFile.read(dataBuffer.byte, blockSize);
			//wait for PC side to be ready to receive
			//Serial.read();
			
			delay(1);
			for( i = 0; i < blockSize; i++ )
			{
				Serial.write((char)dataBuffer.byte[i]);
			}
			
			pos += blockSize;
		}

	}else
	{
		Serial.println(F("error"));
	}
	flashFile.close();
}

/*******************************************************************//**
 *  \brief Write a file to the serial flash
 *  
 *  Usage:
 *  sflwrite rom.bin 4096 2048 xx[0] xx[1] ... xx[2047] /wait/ ... xx[4095] 
 *  Filename must be 12 chars or less (8.3)
 * 
 *  \return Void
 **********************************************************************/
void sfWriteFile()
{
	char *arg;
	uint16_t i, count, blockSize;
	uint32_t fileSize, pos=0;
	char fileName[13]; 		//Max filename length (8.3 plus a null char terminator)

	//get the file name
    arg = SCmd.next();
    i = 0;
    while( (*arg != 0) && ( i < 12) )
    {
		fileName[i++] = *(arg++);
	}
	fileName[i] = 0; //null char terminator

	//get the size in the next argument
    arg = SCmd.next();
    fileSize = strtoul(arg, (char**)0, 0);
    
    //get the blockSize in the next argument
    arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
	Serial.read(); //there's an extra byte here for some reason - discard

	SerialFlash.create(fileName, fileSize);
	flashFile = SerialFlash.open(fileName);
	while( pos < fileSize )
	{
		// fill buffer from USB
		count = 0;
		while( count < blockSize )
		{
			if( Serial.available() )
			{
				dataBuffer.byte[count++] = Serial.read();
			}
		}
		// write buffer to serial flash file
		flashFile.write(dataBuffer.byte, blockSize);
		pos += blockSize;
		Serial.println(F("rdy"));
	}
	flashFile.close();
}

/*******************************************************************//**
 *  \brief List Files in the Serial Flash
 *  
 *  Usage:
 *  sfllist
 *  
 *  \return Void
 **********************************************************************/
void sfListFiles()
{
	//W25Q128FVSIG
	char fileName[64];
    uint32_t fileSize;
    
    SerialFlash.opendir();
    while (1) {
		if (SerialFlash.readdir(fileName, sizeof(fileName), fileSize))
		{
		  Serial.print(fileName);
		  Serial.print(";");
		  Serial.print(fileSize, DEC);
		  Serial.print(",");
		}else 
		{
			Serial.println();
			break; // no more files
		}
	}
}

/*******************************************************************//**
 *  \brief Detects the state of the nCART signal
 *  If a cart asserts the nCART signal it will be detected.
 *  Note that the mode must be set prior to issuing this command
 *  
 *  Usage:
 *  detect
 *  
 *  \return Void
 **********************************************************************/
void _detect()
{
    if(umd.detectCart())
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
void _setMode()
{
    char *arg;
    arg = SCmd.next();
    switch(*arg)
    {
        case 'g':
            umd.setMode(umd.MD);
            Serial.println(F("mode = g")); 
            break;
        case 'p':
            umd.setMode(umd.PC);
            Serial.println(F("mode = p")); 
            break;
        case 't':
            umd.setMode(umd.TG);
            Serial.println(F("mode = t")); 
            break;
        case 'c':
            umd.setMode(umd.CV);
            Serial.println(F("mode = c")); 
            break;
        case 'm':
			umd.setMode(umd.MS);
			Serial.println(F("mode = m"));
			break;
        default:
            Serial.println(F("mode = undefined")); 
            umd.setMode(umd.undefined);
            break;
    }  
}


/*******************************************************************//**
 *  \brief Erases the contents of the cart
 *  Erases the correspoding Flash IC on the cart.
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  erase 0
 *  erase 1 w
 *  
 *  \return Void
 **********************************************************************/
void eraseChip()
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
                umd.eraseChip(true, chip);
                break;
            default:
                break;
        }
    }else
    {
        umd.eraseChip(false, chip);
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
void getFlashID()
{
    char *arg;
    
    uint32_t data;
    
    //read the flash ID
    data = umd.getFlashID();

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
 *  readword 0x0000
 *      - returns unformated word
 *  readword 0x0000 h
 *      - returns HEX formatted word with eol
 *  
 *  \return Void
 **********************************************************************/
void readWord()
{
    char *arg;
    uint32_t address=0;
    uint16_t data;
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //read the word
    data = umd.readWord(address);

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
 *  
 *  readbyte 0x0000
 *      - returns unformated byte
 *  readbyte 0x0000 h
 *      - returns HEX formatted byte with eol
 *  
 *  \return Void
 **********************************************************************/
void readByte()
{
    char *arg;
    uint32_t address = 0;
    uint8_t data;
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    switch(umd.getMode())
    {
		case umd.MS:
			//calculate effective SMS address in slot 2
			data = umd.readByte(umd.setSMSSlotRegister(2, address), true);
			break;
		default:
			data = umd.readByte(address, true);
			break;
	}
	

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
void readByteBlock()
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
	
	switch(umd.getMode())
    {
		case umd.MS:

			for( i = 0; i < blockSize; i++ )
			{
				//calculate effective SMS address in slot 2
				data = umd.readByte(umd.setSMSSlotRegister(2, address++), true);
				Serial.write((char)(data));	
			}
			break;
			
		default:
			for( i = 0; i < blockSize; i++ )
			{
				data = umd.readByte(address++, true);
				Serial.write((char)(data));
			}
			break;
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
void readSRAMByteBlock()
{
    char *arg;
    uint32_t address = 0;
    uint16_t smsAddress,blockSize = 0, i;
    uint8_t data;

    //get the address in the next argument
	arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
	
	switch(umd.getMode())
    {
		case umd.MS:
			
			//enable the corresponding RAM bank in slot 2
			smsAddress = ((uint16_t)address & 0x3FFF) | umd.SMS_SLOT_2_ADDR;
			umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x88);
			for( i = 0; i < blockSize; i++ )
			{
				//calculate effective SMS address in slot 2
				data = umd.readByte(smsAddress++ , true);
				Serial.write((char)(data));	
			}
			//disable RAM Bank
			umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x00);
			break;
			
		default:
			break;
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
void readWordBlock()
{
    char *arg;
    uint32_t address = 0;
    uint16_t blockSize = 0, i;
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
		data = umd.readWord(address);
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
void writeByte()
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
	umd.writeByte(address, data);
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
void writeWord()
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
	umd.writeWord(address, data);
}

/*******************************************************************//**
 *  \brief Program a byte in the cartridge. Prior to progamming,
 *  the sector or entire chip must be erased. The function
 *  returns immediately without checking if the operation
 *  has completed (i.e. toggle bit)
 *  
 *  progbyte 0x0000 0x12
 *      - programs 0x12 into address 0x0000
 * 
 *  progbyte 412 12
 *      - programs decimal 12 into address decimal 412
 *  
 *  \return Void
 **********************************************************************/
void programByte()
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
    if( umd.getMode() == umd.CV )
    {
		address = (uint16_t)(address);      
    }
    
    umd.programByte(address, data, false);
    
    //check if we should verify the write
    arg = SCmd.next();
	if( arg != NULL )
    {
        switch(*arg)
        {
			case 'v':
				delayMicroseconds(50);
				readBack = umd.readByte(address, true);
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
 * \brief
 * Usage:
 
 * \return Void
 **********************************************************************/
void readSRAMWordBlock()
{
    char *arg;
    uint32_t address = 0;
    uint16_t blockSize = 0, i;
    uint16_t data;

    //get the address in the next argument
	arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
	
	umd.writeByteTime(0,3);
	
	//read words from block, output converts to little endian
    for( i = 0; i < blockSize; i += 2 )
    {
		data = umd.readWord(address);
		address += 2;
		Serial.write((char)(data));
        Serial.write((char)(data>>8));
	}
	
	umd.writeByteTime(0,0);
}

/*******************************************************************//**
 * \brief
 * Usage:
 
 * \return Void
 **********************************************************************/
void writeSRAMByteBlock()
{
    char *arg;
    uint32_t address=0;
    uint16_t blockSize, smsAddress, count=0;
	        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
    //receive size bytes
    Serial.read(); //there's an extra byte here for some reason - discard
    
    while( count < blockSize )
    {
		if( Serial.available() )
		{
			dataBuffer.byte[count++] = Serial.read();
		}
	}
	
	SCmd.clearBuffer();
	count = 0;
	switch(umd.getMode())
    {
		case umd.MD:
			//enable the ram latch
			umd.writeByteTime(0,3);
			
			// only odd bytes are valid for Genesis, start at 1 and inc by 2
			for( count=1; count < blockSize ; count += 2 )
			{
				umd.writeByte( address, dataBuffer.byte[count]);
				address += 2;
			}
			//disable the ram latch
			umd.writeByteTime(0,0);
			break;
		case umd.MS:
			//enable the corresponding RAM bank in slot 2
			smsAddress = ((uint16_t)address & 0x3FFF) | umd.SMS_SLOT_2_ADDR;
			umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x88);
			for( count=0; count < blockSize ; count++ )
			{
				umd.writeByte(smsAddress++, dataBuffer.byte[count]);
			}
			//disable RAM Bank
			umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x00);
			break;
		default:
			break;
	}
	
	//Serial.println(address,HEX);
	Serial.println(F("done"));
}

/*******************************************************************//**
 * \brief
 * Usage:
 
 * \return Void
 **********************************************************************/
void writeBRAMByteBlock()
{
    char *arg;
    uint32_t address=0;
    uint16_t blockSize, count=0;
	        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
    //receive size bytes
    Serial.read(); //there's an extra byte here for some reason - discard
    
    while( count < blockSize )
    {
		if( Serial.available() )
		{
			dataBuffer.byte[count++] = Serial.read();
		}
	}
	
	SCmd.clearBuffer();
	count = 0;
	
	//enable write latch for CD BRAM
	umd.writeByte( (uint32_t)0x700000, 0xFF );
	
	// only odd bytes are valid for Genesis, start at 1 and inc by 2
	for( count=1; count < blockSize ; count += 2 )
	{
		umd.writeByte( address, dataBuffer.byte[count]);
		address += 2;
	}

	//disable write latch for CD BRAM
	umd.writeByte( (uint32_t)0x700000, 0x00 );

	//Serial.println(address,HEX);
	Serial.println(F("done"));
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
void programByteBlock()
{
    char *arg;
    uint32_t address=0;
    uint16_t blockSize, count=0;
	        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
    //receive size bytes
    Serial.read(); //there's an extra byte here for some reason - discard
    
    while( count < blockSize )
    {
		if( Serial.available() )
		{
			dataBuffer.byte[count++] = Serial.read();
		}
	}
	
	SCmd.clearBuffer();
	
	//program size bytes
	count = 0;
	while( count < blockSize )
	{
		umd.programByte(address, dataBuffer.byte[count++], true);
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
void programWord()
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
    
    umd.programWord(address, data, false);
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
void programWordBlock()
{
    char *arg;
    uint32_t address=0;
    uint16_t blockSize, count=0;
	        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
    //receive size bytes
    Serial.read(); //there's an extra byte here for some reason - discard
    
    while( count < blockSize )
    {
		if( Serial.available() )
		{
			dataBuffer.byte[count++] = Serial.read();
		}
	}
	
	SCmd.clearBuffer();
	
	//program size/2 words
	count = 0;
	while( count < ( blockSize >> 1) )
	{
		umd.programWord(address, dataBuffer.word[count++], true);
		address += 2;
	}
	
	Serial.println(F("done"));
}

