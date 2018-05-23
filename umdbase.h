/*******************************************************************//**
 *  \file umdbase.h
 *  \author René Richard
 *  \brief This program allows to read and write to various game cartridges 
 *         including: Genesis, Coleco, SMS, PCE - with possibility for 
 *         future expansion.
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

#ifndef umdbase_h
#define umdbase_h

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

/*******************************************************************//** 
 * \class umdbase
 * \brief Teensy umd class to read and write db Flash Carts
 **********************************************************************/
class umdbase
{
    public:
    
        //pin numbers UI
        static const uint8_t nLED = 8;                      ///< LED pin number
        static const uint8_t nPB = 9;                       ///< Pushbutton pin number
    
        struct _flashID {
            uint8_t manufacturer;
            uint8_t device;
            uint8_t type;
            uint32_t size;
        } flashID;
    
        /*******************************************************************//**
         * \brief Constructor
         **********************************************************************/
        umdbase();
    
        /*******************************************************************//**
         * \brief Read the Manufacturer and Product ID in the Flash IC
         * \param alg The algorithm to use, SST 5V devices are different
         * \return void
         **********************************************************************/
        virtual void getFlashID(uint8_t alg);
        
        /*******************************************************************//**
         * \brief calculate the ROM's checksum, every system must define
         * \return void
         **********************************************************************/
        virtual uint16_t calculateChecksum() = 0;
        
        
        /*******************************************************************//**
         * \name Read Functions
         * This group of functions perform various read operations
         **********************************************************************/
        /**@{*/
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
         * \brief Write a word to a 24bit address
         * \param address 24bit address
         * \param data word
         * \return void
         **********************************************************************/
        virtual void writeWord(uint32_t address, uint16_t data);
        
        /**@}*/

    private:
    
        uint8_t _resetPin;
    
        /*******************************************************************//**
         * \brief latch a 16bit address
         * \return void
         **********************************************************************/
        virtual inline void _latchAddress(uint16_t address);
        
        /*******************************************************************//**
         * \brief latch a 24bit address
         * \return void
         **********************************************************************/
        virtual inline void _latchAddress(uint32_t address);
        
        /*******************************************************************//**
         * \brief set the databus port as input
         * \return void
         **********************************************************************/
        inline void _setDatabusInput();
        
        /*******************************************************************//**
         * \brief set the databus port as output
         * \return void
         **********************************************************************/
        inline void _setDatabusOutput();
}

#endif
