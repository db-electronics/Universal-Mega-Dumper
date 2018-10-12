/*******************************************************************//**
 *  \file Sketch.ino
 *  \author René Richard
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

#include <SoftwareSerial.h>         
#include <SerialCommand.h>                  // https://github.com/db-electronics/ArduinoSerialCommand
#include <SerialFlash.h>                    // https://github.com/PaulStoffregen/SerialFlash
#include <SPI.h>

#include "src/umdbase.h"
#include "src/cartfactory.h"

#define DATA_BUFFER_SIZE            2048    ///< Size of serial receive data buffer

SerialCommand SCmd;                         ///< Receive and parse serial commands
umdbase *cart;                              ///< Pointer to all cartridge classes

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

    umdbase::initialize();

    //flash to show we're alive
    for( i=0 ; i<2 ; i++ )
    {
        digitalWrite(umdbase::nLED, LOW);
        delay(100);
        digitalWrite(umdbase::nLED, HIGH);
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
    SCmd.addCommand("checksum", calcChecksum);
    SCmd.addCommand("romsize", getRomSize);
    
    //read commands
    //SCmd.addCommand("rdbyte", readByte);
    SCmd.addCommand("rdbblk", readByteBlock);
    //SCmd.addCommand("rdword", readWord);
    SCmd.addCommand("rdwblk", readWordBlock);
    
    //write commands
    //SCmd.addCommand("wrbyte", writeByte);
    SCmd.addCommand("wrsblk", writeSRAMByteBlock);
    //SCmd.addCommand("wrbrblk",writeBRAMByteBlock);
    //SCmd.addCommand("wrword", writeWord);
    
    //program commands
    SCmd.addCommand("prgwblk",programWordBlock);
    
    //register callbacks for SerialCommand related to the onboard serial flash
    SCmd.addCommand("sfgetid",sfGetID);
    SCmd.addCommand("sfsize", sfGetSize);
    SCmd.addCommand("sferase",sfEraseAll);
    SCmd.addCommand("sfburn", sfBurnCart);
    SCmd.addCommand("sfread", sfReadFile);
    SCmd.addCommand("sfwrite",sfWriteFile);
    SCmd.addCommand("sflist", sfListFiles);
    SCmd.addCommand("sfverify", sfVerify);
    
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
    // We need a cart factory but only one, and this function is the only one that needs to update
    // the cart ptr.  So we can use the static keyword to keep this across calls to the function
    static CartFactory cf;
    
    char *arg;
    uint8_t mode, alg;
    
    // this is the cart type
    arg = SCmd.next();
    mode = (uint8_t)strtoul(arg, (char**)0, 0);

    cart = cf.getCart(static_cast<CartFactory::Mode>(mode));
    if (mode <= cf.getMaxCartMode()){
        Serial.print(F("mode = "));
        Serial.println(arg[0]);

        // next arg, if present, specificies the flash alg, default to 0
        arg = SCmd.next();
        if( arg != NULL ){
            alg = (uint8_t)strtoul(arg, (char**)0, 0);
        }else{
            alg = 0;
        }
        cart->setup(alg);

    }else{
        Serial.println(F("mode = undefined"));
    }

    // report algorithm for debug
    Serial.print(F("Alg = "));
    Serial.println(cart->flashID.alg, DEC);

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
                cart->eraseChip(true);
                break;
            default:
                break;
        }
    }else
    {
        cart->eraseChip(false);
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

    digitalWrite(cart->nLED, LOW);

    //check for next argument, if present
    arg = SCmd.next();

    if( arg != NULL ){
        switch(*arg){
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
            //algorightm
            case 'a':
                Serial.write((char)(cart->flashID.alg));
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
    }else{
        // query the chip when no parameters are passed, use the set algorithm
        cart->getFlashID(cart->flashID.alg);
    }

    digitalWrite(cart->nLED, HIGH);
}

/*******************************************************************//**
 * \brief Get the rom size from the cart interface and send it to the PC
 * Usage:
 * romsize
 *   - returns a uint32_t value
 *     
 **********************************************************************/
void getRomSize()
{
    //give rom size to PC
    Serial.println(cart->getRomSize(),DEC);
}

/*******************************************************************//**
 *  \brief Calculate the cartridge specific checksum
 *  
 *  Usage:
 *  checksum
 *    - returns a uint16_t value
 *  
 *  \return Void
 **********************************************************************/
void calcChecksum()
{
    
    //give rom size to PC
    Serial.println(cart->getRomSize(),DEC);
    
    //call cart's checksum command
    cart->calcChecksum();
    
    //return calculated checksum
    Serial.println(cart->checksum.calculated,DEC);

    //return cart's header checksum, if not available just discard this data
    Serial.println(cart->checksum.expected,DEC);

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
    uint16_t blockSize = 0, i;
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
    
    digitalWrite(cart->nLED, LOW);
    

    for( i = 0; i < blockSize; i++ )
    {
        data = cart->readByte(address++);
        Serial.write((char)(data));
    }
    
    digitalWrite(cart->nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Read a block of words from the cartridge
 *  
 *  Usage:
 *  rdwblk 0x0000 128
 *    - returns 128 unformated bytes
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
    
    digitalWrite(cart->nLED, LOW);
    
    if( sramRead )
    {
        cart->enableSram(0);
        //read words from block, output converts to little endian
        for( i = 0; i < blockSize; i += 2 )
        {
            data = cart->readWord(address);
            address += 2;
            Serial.write((char)(data));
            Serial.write((char)(data>>8));
        }
        cart->disableSram(0);
        
    }else if( latchBankRead )
    {
        cart->writeByteTime(0xA130FD, 0x08); // map bank 8 to 0x300000 - 0x37FFFF
        cart->writeByteTime(0xA130FF, 0x09); // map bank 9 to 0x380000 - 0x3FFFFF

        address = 0x300000 + addrOffset; // TODO: Should start at 0x300000 BUG

        //read the rest of the words from bank switched block, output is little endian
        for( i = 0; i < restBlockSize; i += 2 )
        {
            data = cart->readWord(address);
            address += 2;
            Serial.write((char)(data));
            Serial.write((char)(data>>8));
        }

        cart->writeByteTime(0xA130FD, 0x06); // return banks to original state
        cart->writeByteTime(0xA130FF, 0x07); // return banks to original state
    }else
    {
        //read words from block, output is little endian
        for( i = 0; i < blockSize; i += 2 )
        {
            data = cart->readWord(address);
            address += 2;
            Serial.write((char)(data));
            Serial.write((char)(data>>8));
        }
    }

    digitalWrite(cart->nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Read a block from the cartridge
 *  
 *  Usage:
 *  rdwblk 0x0000 128
 *    - returns 128 unformated bytes
 *  
 *  \return Void
 **********************************************************************/
void readBlock()
{
    char *arg;
    bool sramRead = false;
    bool latchBankRead = false;
    uint32_t address = 0, addrOffset = 0;
    uint16_t blockSize = 0, i, restBlockSize = 0;
    uint16_t readWord;
    uint8_t readByte;

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

    digitalWrite(cart->nLED, LOW);

    // 8 bit reads
    if( cart->info.busSize == 8 )
    {
        
    // 16 bit reads
    }else
    {
        if( address + blockSize >= 0x400000)
        {
            latchBankRead = true;
            restBlockSize = blockSize;
            if(address < 0x400000)
            {
                blockSize = 0x400000 - address;
                restBlockSize -= blockSize;
            }else
            {
                addrOffset = address - 0x400000;
                blockSize = 0;
            }
        }
        
        if( sramRead )
        {
            cart->enableSram(0);
            //read words from block, output converts to little endian
            for( i = 0; i < blockSize; i += 2 )
            {
                readWord = cart->readWord(address);
                address += 2;
                Serial.write((char)(readWord));
                Serial.write((char)(readWord>>8));
            }
            cart->disableSram(0);
            
        }else if( latchBankRead )
        {
            cart->writeByteTime(0xA130FD, 0x08); // map bank 8 to 0x300000 - 0x37FFFF
            cart->writeByteTime(0xA130FF, 0x09); // map bank 9 to 0x380000 - 0x3FFFFF

            address = 0x300000 + addrOffset; // TODO: Should start at 0x300000 BUG

            //read the rest of the words from bank switched block, output is little endian
            for( i = 0; i < restBlockSize; i += 2 )
            {
                readWord = cart->readWord(address);
                address += 2;
                Serial.write((char)(readWord));
                Serial.write((char)(readWord>>8));
            }

            cart->writeByteTime(0xA130FD, 0x06); // return banks to original state
            cart->writeByteTime(0xA130FF, 0x07); // return banks to original state
        }else
        {
            //read words from block, output is little endian
            for( i = 0; i < blockSize; i += 2 )
            {
                readWord = cart->readWord(address);
                address += 2;
                Serial.write((char)(readWord));
                Serial.write((char)(readWord>>8));
            }
        }
    }

    digitalWrite(cart->nLED, HIGH);
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
    uint16_t blockSize, count=0;
            
    //get the address in the next argument
    arg = SCmd.next();
    address = strtoul(arg, (char**)0, 0);
    
    //get the size in the next argument
    arg = SCmd.next();
    blockSize = strtoul(arg, (char**)0, 0);
    
    digitalWrite(cart->nLED, LOW);
    
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
    
    //enable the sram
    cart->enableSram(0);
    
    //sram writes different for 8bit and 16bit busses
    if( cart->info.busSize == 16 )
    {
        //Genesis SRAM only on odd bytes
        if( cart->info.cartType == umdbase::e_carttype::GENESIS )
        {
            for( count=1; count < blockSize ; count += 2 )
            {
                cart->writeByte( address, dataBuffer.byte[count]);
                address += 2;
            }
        }
    }else
    {
        for( count=0; count < blockSize ; count++ )
        {
            cart->writeByte( address++, dataBuffer.byte[count]);
        }
    }
    
    cart->disableSram(0);
    
    Serial.println(F("done"));
    digitalWrite(cart->nLED, HIGH);
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
    
    digitalWrite(cart->nLED, LOW);
    
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
        cart->programWord(address, dataBuffer.word[count++], true);
        address += 2;
    }
    
    Serial.println(F("done"));
    
    digitalWrite(cart->nLED, HIGH);
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
                    digitalWrite(cart->nLED, LOW);
                    delay(250);
                    digitalWrite(cart->nLED, HIGH);
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
    
    digitalWrite(cart->nLED, LOW);
    
    flashFile = SerialFlash.open(fileName);
    if (flashFile)
    {
        Serial.println(F("found"));
        fileSize = flashFile.size();
        Serial.println(fileSize,DEC);
        
        while( pos < fileSize )
        {
            flashFile.read(dataBuffer.byte, blockSize);
            
            if( cart->info.busSize == 16 )
            {
                for( i=0 ; i < ( blockSize >> 1) ; i++ )
                {
                    cart->programWord(address, dataBuffer.word[i], true);
                    address += 2;
                }
            }else
            {
                for( i=0 ; i < blockSize ; i++ )
                {
                    cart->programByte(address, dataBuffer.byte[i], true);
                    address++;
                }
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
    digitalWrite(cart->nLED, HIGH);
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
    
    digitalWrite(cart->nLED, LOW);
    
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
    
    digitalWrite(cart->nLED, HIGH);
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
    
    digitalWrite(cart->nLED, LOW);
    
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
    
    digitalWrite(cart->nLED, HIGH);
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
    
    digitalWrite(cart->nLED, LOW);
    
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
    
    digitalWrite(cart->nLED, HIGH);
}

/*******************************************************************//**
 *  \brief Verify cartridge against a serial flash file
 *  
 *  Usage:
 *  sfverify file.bin
 *  
 *  \return Void
 **********************************************************************/
void sfVerify()
{
    char *arg;
    uint8_t readByte;
    uint16_t i=0, readWord;
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
    
    digitalWrite(cart->nLED, LOW);
    
    flashFile = SerialFlash.open(fileName);
    if (flashFile)
    {
        Serial.println(F("found"));
        fileSize = flashFile.size();
        Serial.println(fileSize,DEC);
        
        while( pos < fileSize )
        {
            flashFile.read(dataBuffer.byte, DATA_BUFFER_SIZE);
            
            if( cart->info.busSize == 16 )
            {
                for( i = 0; i < DATA_BUFFER_SIZE/2 ; i++ )
                {
                    readWord = cart->readWord(pos);
                    if( dataBuffer.word[i] != readWord )
                    {
                        //throw some error
                        Serial.print("$");
                        Serial.println(pos, DEC);
                        Serial.println(dataBuffer.word[i], DEC);
                        Serial.println(readWord, DEC);
                    }
                    pos += 2;
                }
            }else
            {
                for( i = 0; i < DATA_BUFFER_SIZE; i++ )
                {
                    readByte = cart->readByte(pos);
                    if( dataBuffer.byte[i] != readByte )
                    {
                        //throw some error
                        Serial.print("$");
                        Serial.println(pos, DEC);
                        Serial.println(dataBuffer.byte[i], DEC);
                        Serial.println(readByte, DEC);
                    }
                    pos++;
                }
                
            }
            
            //PC side app expects a "." before timeout
            Serial.print(".");
        }

    }else
    {
        Serial.println(F("error"));
    }
    
    flashFile.close();
    
    //Send something other than a "." to indicate we are done
    Serial.print("!");
    
    digitalWrite(cart->nLED, HIGH);
}
