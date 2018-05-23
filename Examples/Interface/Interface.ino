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
#include <umd.h>

#define DATA_BUFFER_SIZE            2048    ///< Size of serial receive data buffer

SerialCommand SCmd;                         ///< Receive and parse serial commands
umd umd;                                    ///< Universal Mega Dumper declaration

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

    umd.setMode(umd.undefined);

    //flash to show we're alive
    for( i=0 ; i<2 ; i++ )
    {
        digitalWrite(umd.nLED, LOW);
        delay(100);
        digitalWrite(umd.nLED, HIGH);
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
    SCmd.addCommand("erase",  eraseChip);
    SCmd.addCommand("getid",  getFlashID);
    SCmd.addCommand("chksum", calcChecksum);
    
    //read commands
    SCmd.addCommand("rdbyte", readByte);
    SCmd.addCommand("rdbblk", readByteBlock);
    SCmd.addCommand("rdword", readWord);
    SCmd.addCommand("rdwblk", readWordBlock);
    
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
        case 1:
            umd.setMode(umd.CV);
            Serial.println(F("mode = 1")); 
            break;
        case 2:
            umd.setMode(umd.MD);
            Serial.println(F("mode = 2")); 
            break;
        case 3:
            umd.setMode(umd.MS);
            Serial.println(F("mode = 3")); 
            break;
        case 4:
            umd.setMode(umd.PC);
            Serial.println(F("mode = 4")); 
            break;
        case 5:
            umd.setMode(umd.TG);
            Serial.println(F("mode = 5"));
            break;
        case 6:
            umd.setMode(umd.SN);
            Serial.println(F("mode = 6"));
            break;
        case 7:
            umd.setMode(umd.SNLO);
            Serial.println(F("mode = 6"));
            break;
        default:
            Serial.println(F("mode = undefined")); 
            umd.setMode(umd.undefined);
            break;
    }  
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
    char fileName[13];      //Max filename length (8.3 plus a null char terminator)
    
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    digitalWrite(umd.nLED, HIGH);
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
    char fileName[13];      //Max filename length (8.3 plus a null char terminator)
    
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
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
    char fileName[13];      //Max filename length (8.3 plus a null char terminator)

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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Erases the contents of the cart
 *  Requires set mode to be issued prior.
 *  
 *  Usage:
 *  erase w
 *  
 *  \return Void
 **********************************************************************/
void eraseChip()
{
    char *arg;

    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            //wait for operation to complete, measure time
            case 'w':
                umd.eraseChip(true);
                break;
            default:
                break;
        }
    }else
    {
        umd.eraseChip(false);
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
    uint8_t i;
    char *arg;

    switch(umd.getMode())
    {
        case umd.MD:
            //check for next argument, if present
            arg = SCmd.next();
            if( arg != NULL )
            {
                switch(*arg)
                {
                    //manufacturer
                    case 'm':
                        Serial.write((char)(umd.flashID.manufacturer));
                        break;
                    //device
                    case 'd':
                        Serial.write((char)(umd.flashID.device));
                        break;
                    //boot mode
                    case 't':
                        Serial.write((char)(umd.flashID.type));
                        break;
                    //size
                    case 's':
                        Serial.write((char)(umd.flashID.size));
                        Serial.write((char)(umd.flashID.size>>8));
                        Serial.write((char)(umd.flashID.size>>16));
                        Serial.write((char)(umd.flashID.size>>24));
                        break;
                    default:
                        break;
                }
            }else
            {
                // query the chip when no paramters are passed
                umd.getFlashID();
            }
            
            break;
            
        default:
            // flash ID contained in array, could be one or many chips
            // PC side listens until end of line
            
            //EOL
            Serial.println();
            break;
    }


}

/*******************************************************************//**
 *  \brief Calculate the checksum of the cartridge
 *  Requires set mode to be issued prior.
 *  
 *  
 *  \return Void
 **********************************************************************/
void calcChecksum()
{
    uint32_t romSize, address;
    
    switch(umd.getMode())
    {
        case umd.MD:
            
            uint16_t headerSum, calcSum; 
            
            headerSum = umd.readWord( 0x00018E );
            romSize = umd.readWord( 0x0001A4 );
            romSize <<= 16;
            romSize |= umd.readWord ( 0x0001A6 );
            romSize += 1;
            
            calcSum = 0;
            address = 0x100;
            
            while( address < romSize )
            {
                calcSum += umd.readWord(address);
                address += 2;
            }
            
            Serial.write((char)(calcSum));
            Serial.write((char)(calcSum>>8));
            
            break;
        default:
            break;
    }
}

/*******************************************************************//**
 *  \brief Read a byte from the cartridge
 *  
 *  rdbyte 0x0000
 *      - returns unformated byte
 *  
 *  \return Void
 **********************************************************************/
void readByte()
{
    char *arg;
    uint32_t address = 0;
    uint8_t data = 0;
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //check for next argument, if present
    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            //read SRAM byte
            case 's':
                switch(umd.getMode())
                {
                    case umd.MS:
                        //enable the corresponding RAM bank in slot 2
                        umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x88);
                        //calculate effective SMS address in slot 2
                        data = umd.readByte(umd.setSMSSlotRegister(2, address), true);
                        //disable RAM Bank
                        umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x00);
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
        
    }else
    {
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
    }
    
    Serial.write((char)(data));
}

/*******************************************************************//**
 *  \brief Read a word from the cartridge
 *  Read a 16bit word from the cartridge, only valid for 16bit buses.
 *  
 *  readword 0x0000
 *      - returns unformated word
 *  
 *  \return Void
 **********************************************************************/
void readWord()
{
    char *arg;
    bool sramRead = false;
    uint32_t address=0;
    uint16_t data;
    
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //check for next argument, if present, for type of read
    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            case 's':
                sramRead = true;
                break;
            default:
                break;
        }
    }
    
    digitalWrite(umd.nLED, LOW);
    
    if( sramRead )
    {
        //read the word
        umd.writeByteTime(0,3);
        data = umd.readWord(address);
        Serial.write((char)(data));
        Serial.write((char)(data>>8));
        umd.writeByteTime(0,0);
    }else
    {
        //read the word
        data = umd.readWord(address);
        Serial.write((char)(data));
        Serial.write((char)(data>>8));
    }

    digitalWrite(umd.nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Read a block of bytes from the cartridge
 *  Read an 8bit byte block from the cartridge. In Coleco mode
 *  the address is forced to uint16_t.
 *  
 *  Usage:
 *  rdbblk 0x0000 128
 *    - returns 128 bytes starting at address 0x0000
 *  
 *  \return Void
 **********************************************************************/
void readByteBlock()
{
    char *arg;
    bool sramRead = false;
    bool bankedPCE = false;
    uint32_t address = 0;
    uint16_t smsAddress, blockSize = 0, i;
    uint8_t data, bank = 0;

    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
    
    //check for next argument, if present, for type of read
    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            case 's':
                sramRead = true;
                break;
            case 'b':
                bankedPCE = true;
                break;
            default:
                break;
        }
    }
    
    digitalWrite(umd.nLED, LOW);
    
    switch(umd.getMode())
    {
        case umd.MS:
            if( sramRead )
            {
                //enable the corresponding RAM bank in slot 2
                smsAddress = ((uint16_t)address & 0x3FFF) | umd.SMS_SLOT_2_ADDR;
                umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR,0x88);
                for( i = 0; i < blockSize; i++ )
                {
                    //calculate effective SMS address in slot 2
                    data = umd.readByte(smsAddress++, true);
                    Serial.write((char)(data)); 
                }
                //disable RAM Bank
                umd.writeByte((uint16_t)umd.SMS_CONF_REG_ADDR, 0x00);
            }else
            {
                for( i = 0; i < blockSize; i++ )
                {
                    //no banking necessary for addresses up to 0x7FFF
                    if( address < umd.SMS_SLOT_2_ADDR )
                    {
                        data = umd.readByte( (uint16_t)address++ , true);
                        Serial.write((char)(data));
                    }else
                    {
                        //calculate effective SMS address in slot 2
                        data = umd.readByte(umd.setSMSSlotRegister(2, address++), true);
                        Serial.write((char)(data)); 
                    }
                }
            }

            break;
        case umd.PC:
            if(bankedPCE)
            {
                uint32_t currAddress;

                bank = 0;
                currAddress = address;
                for( i = 0; i < blockSize; i++ )
                {
                    // SFII CE Mapper
                    // Bank switching occurs in banks 0x40-0x7F
                    // bank 0x40 ia at 0x80000 in linear ROM space
                    
                    if(currAddress < 0x080000)
                        address = currAddress;
                    else
                    if(currAddress < 0x100000)
                    {
                        if(bank != 1)
                        {
                            umd.writeByte((uint32_t)0x001FF0, 0x00);
                            bank = 1;
                        }
                        address = currAddress;
                    }
                    else
                    if(currAddress < 0x180000)
                    {
                        if(bank != 2)
                        {
                            umd.writeByte((uint32_t)0x001FF1, 0x01);
                            bank = 2;
                        }
                        address = currAddress - 0x80000;
                    }
                    else
                    if(currAddress < 0x200000)
                    {
                        if(bank != 3)
                        {
                            umd.writeByte((uint32_t)0x001FF2, 0x01);
                            bank = 3;
                        }
                        address = currAddress - 0x100000;
                    }
                    else
                    if(currAddress < 0x280000)
                    {
                        if(bank != 4)
                        {
                            umd.writeByte((uint32_t)0x001FF3, 0x01);
                            bank = 4;
                        }
                        address = currAddress - 0x180000;
                    }

                    data = umd.readByte(address, true);
                    Serial.write((char)(data));
                    currAddress ++;
                } 
            }
            else
            {
                data = umd.readByte(address++, true);
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
    
    digitalWrite(umd.nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Read a block of words from the cartridge
 *  
 *  Usage:
 *  rdwblk 0x0000 128
 *    - returns 128 unformated words
 *  
 *  \return Void
 **********************************************************************/
void readWordBlock()
{
    char *arg;
    bool sramRead = false;
    bool latchBankRead = false;
    uint32_t address = 0, addrOffset = 0;
    uint16_t blockSize = 0, i, restBlockSize = 0;
    uint16_t data;

    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next(); 
    blockSize = strtoul(arg, (char**)0, 0);
    
    //check for next argument, if present, for type of read
    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            case 's':
                sramRead = true;
                break;
            default:
                break;
        }
    }

    if( address + blockSize >= 0x400000)
    {
        latchBankRead = true;
        restBlockSize = blockSize;
        if(address < 0x400000)
        {
            blockSize = 0x400000 - address;
            restBlockSize -= blockSize;
        }
        else
        {
            addrOffset = address - 0x400000;
            blockSize = 0;
        }
    }
    
    digitalWrite(umd.nLED, LOW);
    
    if( sramRead )
    {
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
    }else
    {
        //read words from block, output is little endian
        for( i = 0; i < blockSize; i += 2 )
        {
            data = umd.readWord(address);
            address += 2;
            Serial.write((char)(data));
            Serial.write((char)(data>>8));
        }
    }

    if( latchBankRead )
    {
        umd.writeByteTime(0xA130FD, 0x08 ); // map bank 8 to 0x300000 - 0x37FFFF
        umd.writeByteTime(0xA130FF, 0x09 ); // map bank 9 to 0x380000 - 0x3FFFFF

        address = 0x300000 + addrOffset; // TODO: Should start at 0x300000 BUG

        //read the rest of the words from bank switched block, output is little endian
        for( i = 0; i < restBlockSize; i += 2 )
        {
            data = umd.readWord(address);
            address += 2;
            Serial.write((char)(data));
            Serial.write((char)(data>>8));
        }

        umd.writeByteTime(0xA130FD, 0x06 ); // return banks to original state
        umd.writeByteTime(0xA130FF, 0x07 ); // return banks to original state
    }

    digitalWrite(umd.nLED, HIGH);
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
 *  Usage:
 *  writebyte 0x00 0x01 t
 *    - writes 0xFE to address 0x00 by strobing Genesis nTIME signal
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
    
    //check next arg, if present, for write type
    arg = SCmd.next();
    if( arg != NULL )
    {
        switch(*arg)
        {
            //write time byte on Genesis
            case 't':
                umd.writeByteTime((uint16_t)address, data);
                break;
            default:
                break;
        }
    }else
    {
        //write the byte
        umd.writeByte(address, data);
    }
    

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
    uint8_t data;
        
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the data in the next argument
    arg = SCmd.next(); 
    data = (uint8_t)strtoul(arg, (char**)0, 0);

    //if coleco, force 16 bit address program
    if( umd.getMode() == umd.CV )
    {
        address = (uint16_t)(address);      
    }
    
    umd.programByte(address, data, true);
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
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
    
    umd.programWord(address, data, true);
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
    
    digitalWrite(umd.nLED, LOW);
    
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
    
    digitalWrite(umd.nLED, HIGH);
}

