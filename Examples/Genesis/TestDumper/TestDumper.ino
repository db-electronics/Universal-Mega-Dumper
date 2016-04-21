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

  char i;

  //hello PC
  Serial.begin(57600);
  Serial.println(F("db Electronics TeensyDumper v0.1"));

  db.setMode(GENESIS);

  //flash to show we're alive
  for(i=0;i<4;i++)
  {
    digitalWrite(db.nLED, LOW);
    delay(250);
    digitalWrite(db.nLED, HIGH);
    delay(250);
  }

  //register callbacks for SerialCommand
  SCmd.addCommand("DT",dbTD_detectCMD);
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



