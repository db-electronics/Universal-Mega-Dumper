 /*
    Title:          ArduinoTDump.cpp
    Author:         René Richard
    Description:
        This library allows to read and write to various game cartridges
        including: Genesis, SMS, PCE - with possibility for future
        expansion.
    Target Hardware:
        Teensy++2.0 with db Electronics TeensyDumper board
    Arduino IDE settings:
        Board Type  - Teensy++2.0
        USB Type    - Serial
        CPU Speed   - 16 MHz

 LICENSE
 
    This file is part of ArduinoTDump.

    ArduinoTDump is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Arduino.h"
#include "ArduinoTDump.h"

ArduinoTDump::ArduinoTDump() {

  char i;

  //Dataport as inputs, use port access for performance on these
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;

  //74HC373 latch enable input is active high, default to low
  pinMode(ALE_low, OUTPUT);
  digitalWrite(ALE_low, LOW);
  pinMode(ALE_high, OUTPUT);
  digitalWrite(ALE_high, LOW);

  //Cartridge detect pins need to be inputs
  pinMode(nPCD, INPUT);
  pinMode(nGCD, INPUT);
  pinMode(nSCD, INPUT);

  //chip enables as outputs, default to high
  pinMode(nSCE, OUTPUT);
  digitalWrite(nSCE, HIGH);
  pinMode(nGCE, OUTPUT);
  digitalWrite(nGCE, HIGH);
  pinMode(nPCE, OUTPUT);
  digitalWrite(nPCE, HIGH);

  //write signals default to high
  pinMode(nLWR, OUTPUT);    // 8 bit write, low byte (PCE, SMS)
  digitalWrite(nLWR, HIGH);
  pinMode(nUWR, OUTPUT);    // 8 bit write, high byte
  digitalWrite(nUWR, HIGH);
  pinMode(nGWR, OUTPUT);    // 16 bit write, genesis only
  digitalWrite(nGWR, HIGH);

  //read-nOE default to high
  pinMode(nRD, OUTPUT);
  digitalWrite(nRD, HIGH);

  //other signals
  pinMode(nRST, OUTPUT);
  digitalWrite(nRST, LOW);  //start in reset
  pinMode(nTIME, OUTPUT);
  digitalWrite(nTIME, HIGH);
  pinMode(M07, OUTPUT);
  digitalWrite(M07, LOW);

  //wait for things to settle
  pinMode(nLED, OUTPUT);
  for(i=0;i<4;i++)
  {
    digitalWrite(nLED, LOW);
    delay(250);
    digitalWrite(nLED, HIGH);
    delay(250);
  }

  //release from reset
  digitalWrite(nRST, HIGH);

}

unsigned int ArduinoTDump::readWord(unsigned long address)
{
  unsigned int readData;

  dbTD_latchAddress(address);

  //set data bus to inputs
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;

  // read the bus
  digitalWrite(nGCE, LOW);
  digitalWrite(nRD, LOW);
  
  readData = (unsigned int)DATAINH;
  readData <<= 8;
  readData |= (unsigned int)(DATAINL & 0x00FF);
  
  digitalWrite(nGCE, HIGH);
  digitalWrite(nRD, HIGH);

  return readData;
}

void ArduinoTDump::writeWord(unsigned long address, unsigned int data)
{
  dbTD_latchAddress(address);

  //set data bus to outputs
  DATAH_DDR = 0xFF;
  DATAL_DDR = 0xFF;

  //put word on bus
  DATAOUTL = (unsigned char)(data);
  DATAOUTH = (unsigned char)(data>>8);

  // write to the bus
  digitalWrite(nGCE, LOW);
  digitalWrite(nGWR, LOW);
  delayMicroseconds(1);
  digitalWrite(nGWR, HIGH);
  digitalWrite(nGCE, HIGH);
  
  //set data bus to inputs
  DATAH_DDR = 0x00;
  DATAL_DDR = 0x00;
}

void ArduinoTDump::readBlock(unsigned long address, unsigned long blockSize)
{
  unsigned int i;

  for(i=0 ; i < blockSize ; i+=2)
  {
    latchAddress(address);
    //set data bus to inputs
    DATAH_DDR = 0x00;
    DATAL_DDR = 0x00;
    // read the bus
    digitalWrite(nGCE, LOW);
    digitalWrite(nRD, LOW);
    dataBuffer[i] = DATAINH;
    dataBuffer[i+1] = DATAINL;
    digitalWrite(nGCE, HIGH);
    digitalWrite(nRD, HIGH);
    address += 2;
  }
}

void ArduinoTDump::latchAddress(unsigned long address)
{
  unsigned char addrh,addrm,addrl;
  //separate address into 3 bytes for address latches
  addrl = (unsigned char)(address & 0xFF);
  addrm = (unsigned int)(address>>8 & 0xFF);
  addrh = (unsigned int)(address>>16 & 0xFF);

  //set data to outputs
  DATAH_DDR = 0xFF;
  DATAL_DDR = 0xFF;

  //put low and mid address on bus and latch it
  DATAOUTH = addrm;
  DATAOUTL = addrl;
  digitalWrite(ALE_low, HIGH);
  digitalWrite(ALE_low, LOW);

  //put high address on bus and latch it
  DATAOUTH = 0x00;
  DATAOUTL = addrh;
  digitalWrite(ALE_high, HIGH);
  digitalWrite(ALE_high, LOW);
}

unsigned int ArduinoTDump::getFlashId(char type)
{
  unsigned int flashID;
  switch(type)
  {
    case 'M': //macronix
      writeWordGen(0x555,0xAA);
      writeWordGen(0x2AA,0x55);
      writeWordGen(0x555,0x90);
      flashID = readWordGen(0x01);
      break;
    default:
      flashID = 0xFFFF;
      break;
  }
  
  return flashID;
}

bool ArduinoTDump::detectCart(char type)
{
  bool detect = false;

  switch(type)
  {
    case GENESIS:
      if (digitalRead(nGCD) == LOW)
      {
        detect = true;
      }
      break;
    case SMS:
      if (digitalRead(nSCD) == LOW)
      {
        detect = true;
      }
      break;
    case PCE:
      if (digitalRead(nPCD) == LOW)
      {
        detect = true;
      }
      break;
    default:

      break;
  }
}

