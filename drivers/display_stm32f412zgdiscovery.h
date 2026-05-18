/***************************************************************************
 *   Copyright (C) 2010, 2011 by Terraneo Federico                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

#ifdef _BOARD_STM32F412ZG_DISCOVERY

#include "mxgui_settings.h"
#include "display.h"
#include "point.h"
#include "color.h"
#include "font.h"
#include "image.h"
#include "iterator_direction.h"
#include "misc_inst.h"
#include "line.h"
#include "miosix.h"
#include "board_settings.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <array>

namespace mxgui
{
class DisplayImpl : public Display
{
public:
        /**
     * \return an instance to this class (singleton)
     */
    static DisplayImpl& instance();
    
    /**
     * Turn the display On after it has been turned Off.
     * Display initial state is On.
     */
    void doTurnOn() override;

    /**
     * Turn the display Off. It can be later turned back On.
     */
    void doTurnOff() override;
    
    /**
     * Set display brightness. Depending on the underlying driver,
     * may do nothing.
     * \param brt from 0 to 100
     */
    void doSetBrightness(int brt) override;
    
    /**
     * \return a pair with the display height and width
     */
    std::pair<short int, short int> doGetSize() const override;

    /**
     * Write text to the display. If text is too long it will be truncated
     * \param p point where the upper left corner of the text will be printed
     * \param text, text to print.
     */
    void write(Point p, const char *text) override;

    /**
     *  Write part of text to the display
     * \param p point of the upper left corner where the text will be drawn.
     * Negative coordinates are allowed, as long as the clipped view has
     * positive or zero coordinates
     * \param a Upper left corner of clipping rectangle
     * \param b Lower right corner of clipping rectangle
     * \param text text to write
     */
    void clippedWrite(Point p, Point a, Point b, const char *text) override;

    /**
     * Clear the Display. The screen will be filled with the desired color
     * \param color fill color
     */
    void clear(Color color) override;

    /**
     * Clear an area of the screen
     * \param p1 upper left corner of area to clear
     * \param p2 lower right corner of area to clear
     * \param color fill color
     */
    void clear(Point p1, Point p2, Color color) override;

    /**
     * This backend does not require it, so it is a blank.
     */
    void beginPixel() override;

    /**
     * Draw a pixel with desired color. You have to call beginPixel() once
     * before calling setPixel()
     * \param p point where to draw pixel
     * \param color pixel color
     */
    void setPixel(Point p, Color color) override;

    /**
     * Draw a line between point a and point b, with color c
     * \param a first point
     * \param b second point
     * \param c line color
     */
    void line(Point a, Point b, Color color) override;

    /**
     * Draw an horizontal line on screen.
     * Instead of line(), this member function takes an array of colors to be
     * able to individually set pixel colors of a line.
     * \param p starting point of the line
     * \param colors an array of pixel colors whoase size must be b.x()-a.x()+1
     * \param length length of colors array.
     * p.x()+length must be <= display.width()
     */
    void scanLine(Point p, const Color *colors, unsigned short length) override;
    
    /**
     * \return a buffer of length equal to this->getWidth() that can be used to
     * render a scanline.
     */
    Color *getScanLineBuffer() override;
    
    /**
     * Draw the content of the last getScanLineBuffer() on an horizontal line
     * on the screen.
     * \param p starting point of the line
     * \param length length of colors array.
     * p.x()+length must be <= display.width()
     */
    void scanLineBuffer(Point p, unsigned short length) override;

    /**
     * Draw an image on the screen
     * \param p point of the upper left corner where the image will be drawn
     * \param i image to draw
     */
    void drawImage(Point p, const ImageBase& img) override;

    /**
     * Draw part of an image on the screen
     * \param p point of the upper left corner where the image will be drawn.
     * Negative coordinates are allowed, as long as the clipped view has
     * positive or zero coordinates
     * \param a Upper left corner of clipping rectangle
     * \param b Lower right corner of clipping rectangle
     * \param i Image to draw
     */
    void clippedDrawImage(Point p, Point a, Point b, const ImageBase& img) override;

    /**
     * Draw a rectangle (not filled) with the desired color
     * \param a upper left corner of the rectangle
     * \param b lower right corner of the rectangle
     * \param c color of the line
     */
    void drawRectangle(Point a, Point b, Color c) override;
    
    /**
     * Pixel iterator. A pixel iterator is an output iterator that allows to
     * define a window on the display and write to its pixels.
     */
    class pixel_iterator
    {
    public:
        /**
         * Default constructor, results in an invalid iterator.
         */
        pixel_iterator(): pixelLeft(0) {}

        /**
         * Set a pixel and move the pointer to the next one
         * \param color color to set the current pixel
         * \return a reference to this
         */
        pixel_iterator& operator= (Color color)
        {
            pixelLeft--;
            writeData(color);
            return *this;
        }

        /**
         * Compare two pixel_iterators for equality.
         * They are equal if they point to the same location.
         */
        bool operator== (const pixel_iterator& itr)
        {
            return this->pixelLeft==itr.pixelLeft;
        }

        /**
         * Compare two pixel_iterators for inequality.
         * They different if they point to different locations.
         */
        bool operator!= (const pixel_iterator& itr)
        {
            return this->pixelLeft!=itr.pixelLeft;
        }

        /**
         * \return a reference to this.
         */
        pixel_iterator& operator* () { return *this; }

        /**
         * \return a reference to this. Does not increment pixel pointer.
         */
        pixel_iterator& operator++ ()  { return *this; }

        /**
         * \return a reference to this. Does not increment pixel pointer.
         */
        pixel_iterator& operator++ (int)  { return *this; }
        
        /**
         * Must be called if not all pixels of the required window are going
         * to be written.
         */
        void invalidate() {}

    private:
        /**
         * Constructor
         * \param pixelLeft number of remaining pixels
         */
        pixel_iterator(unsigned int pixelLeft): pixelLeft(pixelLeft) {}

        unsigned int pixelLeft; ///< How many pixels are left to draw

        friend class DisplayImpl; //Needs access to ctor
    };

    /**
     * Specify a window on screen and return an object that allows to write
     * its pixels.
     * Note: a call to begin() will invalidate any previous iterator.
     * \param p1 upper left corner of window
     * \param p2 lower right corner (included)
     * \param d increment direction
     * \return a pixel iterator
     */
    pixel_iterator begin(Point p1, Point p2, IteratorDirection d);

    /**
     * \return an iterator which is one past the last pixel in the pixel
     * specified by begin. Behaviour is undefined if called before calling
     * begin()
     */
    pixel_iterator end() const
    {
        //Default ctor: pixelLeft is zero.
        return pixel_iterator();
    }
    
    /**
     * Destructor
     */
    ~DisplayImpl() override;

private:
    /**
     * Constructor.
     * Do not instantiate objects of this type directly from application code.
     */
    DisplayImpl();
    
    /// Although the driver supports a 320x240 display, the one installed on
    /// the chip is only 240x240
    static const short int width=240;
    static const short int height=240;

    #if !(defined MXGUI_ORIENTATION_VERTICAL || \
        defined MXGUI_ORIENTATION_VERTICAL_MIRRORED || \
        defined MXGUI_ORIENTATION_HORIZONTAL || \
        defined MXGUI_ORIENTATION_HORIZONTAL_MIRRORED)
    #error No orientation defined
    #endif

    /**
     * Set cursor to desired location
     * \param point where to set cursor (0<=x<240, 0<=y<320)
     */
    static inline void setCursor(Point p)
    {
        #ifdef MXGUI_ORIENTATION_VERTICAL

        writeReg(0x36);
        writeData(0xA0);

        writeReg(0x2A);
        writeData(0x00 + ((p.x() + 80) >> 8));
        writeData(0x00 + ((p.x() + 80) & 0xFF));
        writeData(0x00 + ((p.x() + 80) >> 8));
        writeData(0x00 + ((p.x() + 80) & 0xFF));

        writeReg(0x2B);
        writeData(0x00 + (p.y() >> 8));
        writeData(0x00 + (p.y() & 0xFF));
        writeData(0x00 + (p.y() >> 8));
        writeData(0x00 + (p.y() & 0xFF));
        #elif defined MXGUI_ORIENTATION_HORIZONTAL
        /// TODO: implement
        #elif defined MXGUI_ORIENTATION_VERTICAL_MIRRORED
        /// TODO: implement
        #else // MXGUI_ORIENTATION_HORIZONTAL_MIRRORED
        /// TODO: implement
        #endif
    }
    
    /**
     * Set a hardware window on the screen, optimized for writing text.
     * The GRAM increment will be set to up-to-down first, then left-to-right
     * which is the correct increment to draw fonts
     * \param p1 upper left corner of the window
     * \param p2 lower right corner of the window
     */
    static inline void textWindow(Point p1, Point p2)
    {
        #ifdef MXGUI_ORIENTATION_VERTICAL
        
        writeReg(0x36);
        writeData(0x80);
        
        writeReg(0x2A);
        writeData(0x00 + (p1.y() >> 8));
        writeData(0x00 + (p1.y() & 0xFF));
        writeData(0x00 + (p2.y() >> 8));
        writeData(0x00 + (p2.y() & 0xFF));

        writeReg(0x2B);
        writeData(0x00 + ((p1.x() + 80) >> 8));
        writeData(0x00 + ((p1.x() + 80) & 0xFF));
        writeData(0x00 + ((p2.x() + 80) >> 8));
        writeData(0x00 + ((p2.x() + 80) & 0xFF));
        #elif defined MXGUI_ORIENTATION_HORIZONTAL
        /// TODO: implement
        #elif defined MXGUI_ORIENTATION_VERTICAL_MIRRORED
        /// TODO: implement
        #else //MXGUI_ORIENTATION_HORIZONTAL_MIRRORED
        /// TODO: implement
        #endif
    }

    /**
     * Set a hardware window on the screen, optimized for drawing images.
     * The GRAM increment will be set to left-to-right first, then up-to-down
     * which is the correct increment to draw images
     * \param p1 upper left corner of the window
     * \param p2 lower right corner of the window
     */
    static inline void imageWindow(Point p1, Point p2)
    {
        #ifdef MXGUI_ORIENTATION_VERTICAL
        writeReg(0x36);
        writeData(0xA0);

        writeReg(0x2A);
        writeData(0x00 + ((p1.x() + 80) >> 8));
        writeData(0x00 + ((p1.x() + 80) & 0xFF));
        writeData(0x00 + ((p2.x() + 80) >> 8));
        writeData(0x00 + ((p2.x() + 80) & 0xFF));

        writeReg(0x2B);
        writeData(0x00 + ((p1.y() + 0) >> 8));
        writeData(0x00 + ((p1.y() + 0) & 0xFF));
        writeData(0x00 + ((p2.y() + 0) >> 8));
        writeData(0x00 + ((p2.y() + 0) & 0xFF));
        #elif defined MXGUI_ORIENTATION_HORIZONTAL
        /// TODO: implement
        #elif defined MXGUI_ORIENTATION_VERTICAL_MIRRORED
        /// TODO: implement
        #else //MXGUI_ORIENTATION_HORIZONTAL_MIRRORED
        /// TODO: implement
        #endif
    }

    /**
     * Memory layout of the display.
     * This display driver assumes the RS (register select) line is wired to
     * address line A0.
     */
    struct DisplayMemLayout
    {
        volatile unsigned short REG;    //Index, select register to write
        volatile unsigned short RAM;    //Ram, read and write from registers and GRAM
    };

    /**
     * Pointer to the memory mapped display.
     */
    static DisplayMemLayout *const DISPLAY;

    /**
     * Set the index register
     * \param reg register to select
     */
    static void writeReg(unsigned char reg)
    {
        DISPLAY->REG=reg;
    }

    /**
     * Write data to selected register
     * \param data data to write
     */
    static void writeData(unsigned short data)
    {
        DISPLAY->RAM=data;
    }

    /**
     * Write data from selected register
     * \return data read from register
     */
    static unsigned short readData()
    {
        return DISPLAY->RAM;
    }
    
    static void enableRccFmc()
    {
        volatile unsigned int* rccFMCAHB3Reg = (volatile unsigned int*)(0x38U + 0x3800U + 0x00020000U + 0x40000000U);
        *rccFMCAHB3Reg |= 0x01U;
        volatile unsigned int dummy = *rccFMCAHB3Reg;
        (void) dummy; // enforce write finished
    }

    static void enableRccGpioBank(unsigned int bankReg)
    {
        volatile unsigned int* rccFMCAHB1Reg = (volatile unsigned int*)(0x30U + 0x3800U + 0x00020000U + 0x40000000U);
        *rccFMCAHB1Reg |= bankReg;
        volatile unsigned int dummy = *rccFMCAHB1Reg;
        (void) dummy; // enforce write finished
    }

    template <unsigned int P, unsigned char N>
    static inline void setGpioAlternateMode()
    {
        miosix::Gpio<P,N>::mode(miosix::Mode::ALTERNATE);
        miosix::Gpio<P,N>::speed(miosix::Speed::VERY_HIGH);
        miosix::Gpio<P,N>::alternateFunction(12);
    }


    static inline void initGpiosFMC()
    {
        /// PD7 is NE1 (Display CS)
        setGpioAlternateMode<GPIOD_BASE, 7>();

        /// Switch 16 miosix::Gpio pins to alternate FMC mode for 16 bit data bus
        setGpioAlternateMode<GPIOD_BASE, 0>();  // DB2
        setGpioAlternateMode<GPIOD_BASE, 1>();  // DB3
        setGpioAlternateMode<GPIOD_BASE, 8>();  // DB13
        setGpioAlternateMode<GPIOD_BASE, 9>();  // DB14
        setGpioAlternateMode<GPIOD_BASE, 10>(); // DB15
        setGpioAlternateMode<GPIOD_BASE, 14>(); // DB0
        setGpioAlternateMode<GPIOD_BASE, 15>(); // DB1

        setGpioAlternateMode<GPIOE_BASE, 7>();  // DB4
        setGpioAlternateMode<GPIOE_BASE, 8>();  // DB5
        setGpioAlternateMode<GPIOE_BASE, 9>();  // DB6
        setGpioAlternateMode<GPIOE_BASE, 10>(); // DB7
        setGpioAlternateMode<GPIOE_BASE, 11>(); // DB8
        setGpioAlternateMode<GPIOE_BASE, 12>(); // DB9
        setGpioAlternateMode<GPIOE_BASE, 13>(); // DB10
        setGpioAlternateMode<GPIOE_BASE, 14>(); // DB11
        setGpioAlternateMode<GPIOE_BASE, 15>(); // DB12

        /// PD4 is FMC_NOE
        setGpioAlternateMode<GPIOD_BASE, 4>();

        /// PD5 is FMC_NWE
        setGpioAlternateMode<GPIOD_BASE, 5>();

        // PF0 is A0 (RS)
        setGpioAlternateMode<GPIOF_BASE, 0>();
    }

    Color *buffer; ///< For scanLineBuffer
};

} //namespace mxgu

#endif
