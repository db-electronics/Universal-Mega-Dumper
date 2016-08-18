/*
    Title:          TestDumper
    Author:         René Richard
    Description:
        This program allows to read and write to various game cartridges
        including: Genesis, SMS, PCE - with possibility for future
        expansion.
    Target Hardware:
        Teensy++2.0 with db Electronics TeensyDumper board rev >= 1.1
    Arduino IDE settings:
        Board Type  - Teensy++2.0
        USB Type    - Serial
        CPU Speed   - 16 MHz
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

void setup() {

  uint8_t i;

  //hello PC
  Serial.begin(57600);
  Serial.println(F("db Electronics TeensyDumper v0.2"));

  db.setMode(db.genesis);

  //flash to show we're alive
  for(i=0;i<4;i++)
  {
    digitalWrite(db.nLED, LOW);
    delay(250);
    digitalWrite(db.nLED, HIGH);
    delay(250);
  }

  //register callbacks for SerialCommand
  SCmd.addCommand("dt",dbTD_detectCMD);
  SCmd.addCommand("sm",dbTD_setModeCMD);
  SCmd.addCommand("er",dbTD_eraseChipCMD);
  SCmd.addCommand("id",dbTD_flashIDCMD);
  SCmd.addCommand("rw",dbTD_readWordCMD);
  SCmd.addCommand("rb",dbTD_readByteCMD);
  SCmd.addCommand("wb",dbTD_writeByteCMD);
  SCmd.addDefaultHandler(unknownCMD);
}

void loop()
{
  SCmd.readSerial();
}

void unknownCMD(const char *command)
{
  Serial.println(F("unknown command"));

  Serial.print(F("Unrecognized command: \""));
  Serial.print(command);
  Serial.println(F("\". Registered Commands:"));
  
  Serial.println(SCmd.getCommandList());  //Returns all registered commands
}

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

void dbTD_eraseChipCMD()
{
  db.eraseChip(); 
}

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
          Serial.print(data,HEX);
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

void dbTD_readWordCMD()
{
  char *arg;
  uint32_t address=0;
  uint16_t data;
  arg = SCmd.next();

  address = strtoul(arg, (char**)0, 0);
  data = db.readWord(address);

  Serial.write((char)(data));
  Serial.write((char)(data>>8));

}

void dbTD_readByteCMD()
{
  char *arg;
  uint32_t address=0;
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
          Serial.print(data,HEX);
        break;
      default:
        break;
    }
  }else
  {
    Serial.write((char)(data));
  }
}

void dbTD_writeByteCMD()
{
  char *arg;
  uint32_t address=0;
  uint8_t data;
  
  arg = SCmd.next();
  address = strtoul(arg, (char**)0, 0);

  arg = SCmd.next(); 
  data = (uint8_t)strtoul(arg, (char**)0, 0);

  db.programByte(address,data);
}

/*
void dbTD_readCMD()
{
  char *arg;
  arg = SCmd.next();
  switch(*arg)
  {
    case GENESIS:
      dbTD_readGenesisCMD();
      break;
    case SMS:

      break;
    case PCE:

      break;
    default:

      break;
  }
}

void dbTD_readGenesisCMD()
{
  char *arg;
  unsigned long address=0,blockSize=0,i;
  unsigned int readWord;
  unsigned char checksum=0;
  arg = SCmd.next();
  switch(*arg)
  {
    case 'B':
      arg = SCmd.next();
      address = strtoul(arg, (char**)0, 0);
      blockSize = 1024;
      dbTD_readBlockGenesis(address,blockSize);
      for(i=0 ; i < blockSize ; i++)
      {
        Serial.write(dataBuffer[i]);
        checksum += dataBuffer[i];
      }
      Serial.write(checksum);
      break;
      
    case 'L':
      arg = SCmd.next();
      address = strtoul(arg, (char**)0, 0);
      blockSize = 16;
      dbTD_readBlockGenesis(address,blockSize);
      for(i=0 ; i < blockSize ; i++)
      {
        Serial.write(dataBuffer[i]);
      }
      break;
      
    case 'W':
      arg = SCmd.next();
      address = strtoul(arg, (char**)0, 0);
      readWord = dbTD_readWordGenesis(address);

      Serial.write((char)(readWord));
      Serial.write((char)(readWord>>8));
      break;
      
    default:
      break;
  }
}
*/



