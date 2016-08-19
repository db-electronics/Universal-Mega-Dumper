/** \file dbDumper.h
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
 
    This file is part of dbDumper.

    dbDumper is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    dbDumper is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with dbDumper.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef dbDumper_h
#define dbDumper_h

#define DEBUG         1

  //define pins
#define DATAOUTH      PORTD
#define DATAOUTL      PORTC
#define DATAINH       PIND
#define DATAINL       PINC
#define DATAH_DDR     DDRD
#define DATAL_DDR     DDRC

/** \class dbDumper
 *  
 *  \brief Teensy dbDumper class to read and write db Flash Carts
 */
class dbDumper
{
	public:
		/** eMode Type
		 *  eMode is used by dbDumper to keep track of which mode is currently set
		 */
		enum eMode
		{ 
			undefined, 	/**< undefined mode */
			coleco, 	/**< ColecoVision mode */
			genesis, 	/**< Genesis Megadrive mode */
			pcengine 	/**< PC Engine TG-16 mode */
		};


		dbDumper();
		
		void setMode(eMode);
		eMode getMode() { return _mode; }

		void resetCart();
		bool detectCart();
		uint16_t getFlashID();

		//read
		uint8_t readByte(uint16_t address);
		uint16_t readWord(uint16_t address);
		uint8_t readByte(uint32_t address);
		uint16_t readWord(uint32_t address);
		void readWordBlock(uint32_t address, uint8_t * buf, uint16_t blockSize);
		void readByteBlock(uint32_t address, uint8_t * buf, uint16_t blockSize);

		//write
		void writeWord(uint32_t address, uint16_t data);
		void writeWord(uint16_t address, uint16_t data);
		void writeByte(uint32_t address, uint8_t data);
		void writeByte(uint16_t address, uint8_t data);
		
		//erase
		void eraseChip(void);
		void eraseSector(uint16_t sectorAddress);
		uint8_t toggleBit(uint8_t attempts);
		
		//program
		void programByte(uint16_t address, uint8_t data, bool wait);
		void programByte(uint32_t address, uint8_t data, bool wait);
		void programWord(uint32_t address, uint16_t data, bool wait);
		
		//pin numbers UI
		static const uint8_t nLED = 8;
		static const uint8_t nPB = 9;

	private:
		uint8_t _resetPin;
		uint16_t _flashID;
		eMode _mode;
	
		inline void _latchAddress(uint16_t address);
		inline void _latchAddress(uint32_t address);
		
		void _colSoftwareIDEntry();
		void _colSoftwareIDExit();
		void _colAddrRangeSet(uint16_t address);
		void _colPinMode();

		//pin numbers address control
		static const uint8_t ALE_low = 26;
		static const uint8_t ALE_high = 27;

		//globally affected pins
		static const uint8_t nRD = 25;	
		static const uint8_t nWR = 24;
		static const uint8_t nCE = 19;
		static const uint8_t nCART = 18;

		//general control pins
		static const uint8_t CTRL0 = 38;
		static const uint8_t CTRL1 = 39;
		static const uint8_t CTRL2 = 40;
		static const uint8_t CTRL3 = 41;
		static const uint8_t CTRL4 = 42;
		static const uint8_t CTRL5 = 43;
		static const uint8_t CTRL6 = 44;
		static const uint8_t CTRL7 = 45;

		//Coleco pin functions
		static const uint8_t COL_nBPRES = 39;
		static const uint8_t COL_nE000 = 38;
		static const uint8_t COL_A16 = 38;
		static const uint8_t COL_nC000 = 40;
		static const uint8_t COL_A15 = 40;
		static const uint8_t COL_nA000 = 41;
		static const uint8_t COL_A14 = 41;
		static const uint8_t COL_n8000 = 43;
		static const uint8_t COL_A13 = 43;	

		//Genesis pin functions
		static const uint8_t GEN_SL1 = 38;
		static const uint8_t GEN_SR1 = 39;
		static const uint8_t GEN_nDTACK = 40;
		static const uint8_t GEN_nCAS2 = 41;
		static const uint8_t GEN_nVRES = 42;
		static const uint8_t GEN_nLWR = 43;
		static const uint8_t GEN_nUWR = 44;
		static const uint8_t GEN_nTIME = 45;
		
		//SPI pins
		static const uint8_t MISOp = 23;
		static const uint8_t MOSIp = 22;
		static const uint8_t SCKp = 21;
		static const uint8_t SCSp = 20;	
};

#endif  //dbDumper_h

