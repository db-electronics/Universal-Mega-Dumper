/*******************************************************************//**
 *  \file umdbase.h
 *  \author René Richard
 *  \brief This program allows to read and write to various game cartridges. 
 *         The UMD base class handles all generic cartridge operations, console
 *         specific operations are handled in derived classes.
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

#ifndef umdv1_h
#define umdv1_h

#define DATAOUTH        PORTD     /**< PORTD used for high byte of databus output */
#define DATAOUTL        PORTC     /**< PORTC used for low byte of databus output */
#define PORTALE         PORTB     /**< PORTB used for address latch control */
#define PORTRD          PORTB
#define PORTWR          PORTB
#define PORTCE          PORTE
#define DATAINH         PIND      /**< PIND used for high byte databus input */
#define DATAINL         PINC      /**< PINC used for low byte databus input */
#define DATAH_DDR       DDRD      /**< DDRD data direction for high byte of databus */
#define DATAL_DDR       DDRC      /**< DDRC data direction for low byte of databus */

// setting DATAOUTx to all 1's fixes the S29GL032 reading problem, this activates the pull-up resistors
#define SET_DATABUS_TO_INPUT() 	\
	DATAH_DDR = 0x00;			\
	DATAL_DDR = 0x00;			\
	DATAOUTH = 0xFF;			\
	DATAOUTL = 0xFF;			\

#define SET_DATABUS_TO_OUTPUT()	\
	DATAH_DDR = 0xFF;			\
    DATAL_DDR = 0xFF;			\
	
/*******************************************************************//** 
 * \class umdbase
 * \brief Teensy umd class to read and write db Flash Carts
 **********************************************************************/
class umdv1
{
    public:
    
        //pin numbers UI
        static const uint8_t nLED = 8;                      ///< LED pin number
        static const uint8_t nPB = 9;                       ///< Pushbutton pin number
    
		/*******************************************************************//**
         * \brief console_e
         * eMode is used by umd to keep track of which mode is currently set
         **********************************************************************/
        enum console_e : uint8_t 
        { 
            UNDEFINED = 0,			/**< Undefined mode */
            GENESIS,         		/**< Genesis Megadrive mode */
            SMS,         			/**< Master System */
            PCE,         			/**< PC Engine mode */
            TG16                    /**< TG-16 mode */
        };
        static const uint8_t CARTS_LEN = 6;

		struct s_info {
			uint8_t bus_size;
			console_e console;
            bool mirrored_bus;
		} info;
    
		/*******************************************************************//**
         * \brief s_flashID
         * flashID stores important data about the flash chip on the cartridge
         **********************************************************************/
        struct s_flashID {
            uint8_t manufacturer;
            uint8_t device;
            uint8_t type;
            uint32_t size;
            uint8_t buffermode;     /**< buffermode = 0 single write, buffermode = 1 buffered write */
        } flashID;
    
		/*******************************************************************//**
         * \brief s_checksum
         * checksum stores checksum data
         **********************************************************************/
		struct s_checksum {
			uint16_t expected;
			uint16_t calculated;
			uint32_t romSize;
		} checksum;
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        static void initialize();

	    virtual ~umdv1();
        
        /*******************************************************************//**
         * \brief setup the UMD's hardware for the current cartridge
         * \return void
         **********************************************************************/
        virtual void setup(uint8_t param);
        
        uint8_t mirror_byte(uint8_t data);
        /*******************************************************************//**
         * \name Cartridge Flash Functions
         * This group of functions is specific to the catridge flash
         **********************************************************************/
        /**@{*/
        /*******************************************************************//**
         * \brief Read the Manufacturer and Product ID in the Flash IC
         * \param alg The algorithm to use, SST 5V devices are different
         * \return void
         **********************************************************************/
        virtual void getFlashID();
        
        /*******************************************************************//**
         * \brief Erase the entire Flash IC
         * \param wait specify whether to wait for the operation to complete before returning
         * \return void
         **********************************************************************/
        virtual void eraseChip(bool wait);
        
        /*******************************************************************//**
         * \brief Perform toggle bit algorithm byte mode
         * \param attempts how many toggle bits to attempt
         * \return the number of times the bit successfully toggled
         **********************************************************************/
        uint8_t toggleBit8(uint8_t attempts);

		/*******************************************************************//**
         * \brief Perform toggle bit algorithm word mode
         * \param attempts how many toggle bits to attempt
         * \return the number of times the bit successfully toggled
         **********************************************************************/
        uint8_t toggleBit16(uint8_t attempts);


		/*******************************************************************//**
         * \brief Perform checksum on the cartridge
         * \return the calculated value
         **********************************************************************/
		virtual void calcChecksum()=0;
        
        /*******************************************************************//**
         * \brief Get the ROM's size
         * \return the ROM's size
         **********************************************************************/
		virtual uint32_t getRomSize()=0;

		/*******************************************************************//**
         * \brief enable SRAM memory access
         * \param param any paramaters to pass, not used by all cartridges
         * \return void
         **********************************************************************/
		virtual void enableSram(uint8_t param)=0;
		
		/*******************************************************************//**
         * \brief disable SRAM memory access
         * \param param any paramaters to pass, not used by all cartridges
         * \return void
         **********************************************************************/
		virtual void disableSram(uint8_t param)=0;

        /**@}*/
        
        /*******************************************************************//**
         * \name Read Functions
         * This group of functions perform various read operations
         **********************************************************************/
        /**@{*/
        /*******************************************************************//**
         * \brief Read a byte from a 16bit address
         * \param address 16bit address
         * \return byte from cartridge
         **********************************************************************/
        virtual uint8_t readByte16(uint16_t address);
        
        /*******************************************************************//**
         * \brief Read a byte from a 24bit address
         * \param address 24bit address
         * \return byte from cartridge
         **********************************************************************/
        virtual uint8_t readByte(uint32_t address);
        
        /*******************************************************************//**
         * \brief Read a word from a 16bit address
         * \param address 16bit address
         * \return word from cartridge
         **********************************************************************/
        virtual uint16_t readWord16(uint16_t address);

        /*******************************************************************//**
         * \brief Read a word from a 24bit address
         * \param address 24bit address
         * \return word from cartridge
         **********************************************************************/
        virtual uint16_t readWord(uint32_t address);
        
        /**@}*/
        
        
        /*******************************************************************//**
         * \name Write Functions
         * This group of functions perform various write operations
         **********************************************************************/
        /**@{*/
        
        /*******************************************************************//**
         * \brief Write a byte to a 16bit address
         * \param address 16bit address
         * \param data byte
         * \return void
         **********************************************************************/
        virtual void writeByte16(uint16_t address, uint8_t data);
        
        /*******************************************************************//**
         * \brief Write a byte to a 24bit address
         * \param address 24bit address
         * \param data byte
         * \return void
         **********************************************************************/
        virtual void writeByte(uint32_t address, uint8_t data);

        virtual void writeByteTime(uint32_t address, uint8_t data);
        
        /*******************************************************************//**
         * \brief Write a word to a 16bit address
         * \param address 16bit address
         * \param data word
         * \return void
         **********************************************************************/
        virtual void writeWord16(uint16_t address, uint16_t data);

        /*******************************************************************//**
         * \brief Write a word to a 24bit address
         * \param address 24bit address
         * \param data word
         * \return void
         **********************************************************************/
        virtual void writeWord(uint32_t address, uint16_t data);
        
        
        /**@}*/
        
        /*******************************************************************//** 
         * \name Program Functions
         * This group of functions perform various write operations
         **********************************************************************/
        /**@{*/
        /*******************************************************************//**
         * \brief Program a byte in the Flash IC
         * \param address 24bit address
         * \param data byte
         * \param wait Wait for completion using toggle bit to return from function
         * \return void
         **********************************************************************/
        virtual void programByte(uint32_t address, uint8_t data, bool wait);
        
        /*******************************************************************//**
         * \brief Program a word in the Flash IC
         * \param address 24bit address
         * \param data word
         * \param wait Wait for completion using toggle bit to return from function
         * \return void
         **********************************************************************/
        virtual void programWord(uint32_t address, uint16_t data, bool wait);
    
        /**@}*/
        
	protected:

        //pin numbers address control

        //globally affected pins
        static const uint8_t ALE_high = 27;                 // PB7
        static const uint8_t ALE_high_setmask = 0b10000000;
        static const uint8_t ALE_high_clrmask = 0b01111111;
        static const uint8_t ALE_low = 26;                  // PB6
        static const uint8_t ALE_low_setmask = 0b01000000;
        static const uint8_t ALE_low_clrmask = 0b10111111;
        static const uint8_t nRD = 25;                      // PB5
        static const uint8_t nRD_setmask = 0b00100000;
        static const uint8_t nRD_clrmask = 0b11011111;
        static const uint8_t nWR = 24;                      // PB4
        static const uint8_t nWR_setmask = 0b00010000;
        static const uint8_t nWR_clrmask = 0b11101111;
        static const uint8_t nCE = 19;                      // PE7
        static const uint8_t nCE_setmask = 0b10000000;
        static const uint8_t nCE_clrmask = 0b01111111;
        
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

        //Turbografx-16 pin functions
        static const uint8_t TG_nRST = 38;

        //Super Nintendo pin functions
        static const uint8_t SN_nRST = 45;
        
        //SPI pins
        static const uint8_t MISOp = 23;
        static const uint8_t MOSIp = 22;
        static const uint8_t SCKp = 21;
        static const uint8_t SCSp = 20;
    
        uint8_t _resetPin;
    
    	/*******************************************************************//**
         * \brief latch a 16bit address
         * \return void
         **********************************************************************/
        void latchAddress16(uint16_t address);
        
        /*******************************************************************//**
         * \brief latch a 24bit address
         * \return void
         **********************************************************************/
        void latchAddress32(uint32_t address);
                
        /*******************************************************************//**
         * \brief Read the Manufacturer and Product ID in the Flash IC
         * \param manufacturer the byte specifying the manufacturer
         * \param device the byte specifying the device
         * \return size the size of the flash in bytes
         **********************************************************************/
        uint32_t getFlashSizeFromID(uint8_t manufacturer, uint8_t device);
    
    
    private:
        
};

#endif
