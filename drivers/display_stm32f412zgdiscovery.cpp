/***************************************************************************
 *   Copyright (C) 2010, 2011 by Federico Terraneo                         *
 *   Copyright (C) 2011 by Yury Kuchura                                    *
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

#ifdef _BOARD_STM32F412ZG_DISCOVERY

#include "display_stm32f412zgdiscovery.h"
#include "board_settings.h"
#include "miosix.h"

using namespace std;
using namespace miosix;

namespace mxgui {

void registerDisplayHook(DisplayManager& dm)
{
    dm.registerDisplay(&DisplayImpl::instance());
}

//
// Class DisplayImpl
//
const short int DisplayImpl::width;
const short int DisplayImpl::height;

DisplayImpl& DisplayImpl::instance()
{
    static DisplayImpl instance;
    return instance;
}

void DisplayImpl::doTurnOn()
{
    // Turn on display
    writeReg(0x29);

    // Sleep out
    writeReg(0x11);
}

void DisplayImpl::doTurnOff()
{
    // Turn off display
    writeReg(0xBD);
    writeData(0xFE);

    // Sleep
    writeReg(0x10);

    delayMs(10);
}

void DisplayImpl::doSetBrightness(int brt) {}

pair<short int, short int> DisplayImpl::doGetSize() const
{
    return make_pair(height,width);
}

void DisplayImpl::write(Point p, const char *text)
{
    font.draw(*this,textColor,p,text);
}

void DisplayImpl::clippedWrite(Point p, Point a, Point b, const char *text)
{
    font.clippedDraw(*this,textColor,p,a,b,text);
}

void DisplayImpl::clear(Color color)
{
    clear(Point(0,0),Point(width-1,height-1),color);
}

void DisplayImpl::clear(Point p1, Point p2, Color color)
{
    imageWindow(p1,p2);
    writeReg(0x2C);//Write to GRAM
    int numPixels=(p2.x()-p1.x()+1)*(p2.y()-p1.y()+1);
    for(int i=0;i<numPixels;i++) writeData(color);
}

void DisplayImpl::beginPixel()
{
    textWindow(Point(0,0),Point(width-1,height-1));//Restore default window
}

void DisplayImpl::setPixel(Point p, Color color)
{
    setCursor(p);
    writeReg(0x2C);//Write to GRAM
    writeData(color);
}

void DisplayImpl::line(Point a, Point b, Color color)
{
    //Horizontal line speed optimization
    if(a.y()==b.y())
    {
        imageWindow(Point(min(a.x(),b.x()),a.y()),
                    Point(max(a.x(),b.x()),a.y()));
        writeReg(0x2C);//Write to GRAM
        int numPixels=abs(a.x()-b.x());
        for(int i=0;i<=numPixels;i++) writeData(color);
        return;
    }
    //Vertical line speed optimization
    if(a.x()==b.x())
    {
        textWindow(Point(a.x(),min(a.y(),b.y())),
                    Point(a.x(),max(a.y(),b.y())));
        writeReg(0x2C);//Write to GRAM
        int numPixels=abs(a.y()-b.y());
        for(int i=0;i<=numPixels;i++) writeData(color);
        return;
    }
    //General case, always works but it is much slower due to the display
    //not having fast random access to pixels
    Line::draw(*this,a,b,color);
}

void DisplayImpl::scanLine(Point p, const Color *colors, unsigned short length)
{
    imageWindow(p, Point { (p.x() + length) % width, p.y() });
    writeReg(0x2C);
    for (auto i{0}; i < length; ++i)
    {
        writeData(colors[i]);
    }
}

Color *DisplayImpl::getScanLineBuffer()
{
    if(buffer==0) buffer=new Color[getWidth()];
    return buffer;
}

void DisplayImpl::scanLineBuffer(Point p, unsigned short length)
{
    scanLine(p,buffer,length);
}

void DisplayImpl::drawImage(Point p, const ImageBase& img)
{
    short int xEnd=p.x()+img.getWidth()-1;
    short int yEnd=p.y()+img.getHeight()-1;
    if(xEnd >= width || yEnd >= height) return;

    const unsigned short *imgData=img.getData();
    if(imgData!=0)
    {
        //Optimized version for memory-loaded images
        imageWindow(p,Point(xEnd,yEnd));
        writeReg(0x2C); //Write to GRAM
        int numPixels=img.getHeight()*img.getWidth();
        for(int i=0;i<=numPixels;i++)
        {
            writeData(imgData[0]);
            imgData++;
        }
    } else img.draw(*this,p);
}

void DisplayImpl::clippedDrawImage(Point p, Point a, Point b, const ImageBase& img)
{
    img.clippedDraw(*this,p,a,b);
}

void DisplayImpl::drawRectangle(Point a, Point b, Color c)
{
    line(a,Point(b.x(),a.y()),c);
    line(Point(b.x(),a.y()),b,c);
    line(b,Point(a.x(),b.y()),c);
    line(Point(a.x(),b.y()),a,c);
}

DisplayImpl::pixel_iterator DisplayImpl::begin(Point p1, Point p2,
        IteratorDirection d)
{
    if(p1.x()<0 || p1.y()<0 || p2.x()<0 || p2.y()<0) return pixel_iterator();
    if(p1.x()>=width || p1.y()>=height || p2.x()>=width || p2.y()>=height)
        return pixel_iterator();
    if(p2.x()<p1.x() || p2.y()<p1.y()) return pixel_iterator();

    if(d==DR) textWindow(p1,p2);
    else imageWindow(p1,p2);
    writeReg(0x2C);//Write to GRAM

    unsigned int numPixels=(p2.x()-p1.x()+1)*(p2.y()-p1.y()+1);
    return pixel_iterator(numPixels);
}

DisplayImpl::~DisplayImpl()
{
    if(buffer) delete[] buffer;
}

DisplayImpl::DisplayImpl(): buffer(0)
{
    
    enableRccFmc();

    enableRccGpioBank(0x08U);
    enableRccGpioBank(0x10U);
    enableRccGpioBank(0x20U);

    /// Setup the non-FMC gpios (backlight, reset, tearing effect)
    Gpio<GPIOF_BASE, 5>::mode(Mode::OUTPUT);

    Gpio<GPIOD_BASE, 11>::mode(Mode::OUTPUT);
    Gpio<GPIOD_BASE, 11>::speed(Speed::HIGH);

    Gpio<GPIOG_BASE, 4>::mode(Mode::INPUT);

    /// Turn on backlight
    Gpio<GPIOF_BASE, 5>::high();

    /// Reset sequence
    Gpio<GPIOD_BASE, 11>::low();
    delayMs(5);

    Gpio<GPIOD_BASE, 11>::high();
    delayMs(10);
    
    Gpio<GPIOD_BASE, 11>::low();
    delayMs(20);

    Gpio<GPIOD_BASE, 11>::high();
    delayMs(10);

    
    initGpiosFMC();

    /// BCR1 and BTR1 registers for FMC
    volatile std::uint32_t& BCR1=FSMC_Bank1->BTCR[0];
    volatile std::uint32_t& BTR1=FSMC_Bank1->BTCR[1];

    // Extended mode
    volatile std::uint32_t& BWTR1=FSMC_Bank1E->BWTR[0];

    // Write burst disabled, Extended mode enabled, Wait signal disabled
    // Write enabled, Wait signal active before wait state, Wrap disabled
    // Burst disabled, Data width 16bit, Memory type SRAM, Data mux enabled,
    // Write FIFO disabled
    BCR1 =  FSMC_BCR1_MWID_0 |
            FSMC_BCR1_WREN |
            FSMC_BCR1_EXTMOD |
            FSMC_BCR1_WFDIS;

    BTR1 =  (9 << FSMC_BTR1_ADDSET_Pos) |
            (1 << FSMC_BTR1_ADDHLD_Pos) |
            (36 << FSMC_BTR1_DATAST_Pos) |
            (1 << FSMC_BTR1_BUSTURN_Pos) |
            (1 << FSMC_BTR1_CLKDIV_Pos);

    BWTR1 = (1 << FSMC_BWTR1_ADDSET_Pos) |
            (1 << FSMC_BWTR1_ADDHLD_Pos) |
            (7 << FSMC_BWTR1_DATAST_Pos);
    BCR1 |= FSMC_BCR1_MBKEN;

    delayMs(10);

    //=======================================================================
    // The voodoo begins here. Don't touch the code unless you fully realize
    // what you are doing: the display may be seriously damaged.
    //=======================================================================
    writeReg(0x10);
    delayMs(10);

    writeReg(0x01);
    delayMs(200);

    writeReg(0x11);
    delayMs(120);

    writeReg(0x36);
    writeData(0x00);

    writeReg(0x3A);
    writeData(0x05);

    writeReg(0x21);

    writeReg(0x2A);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0xEF);

    writeReg(0x2B);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0xEF);

    writeReg(0xB2);
    writeData(0x0C);
    writeData(0x0C);
    writeData(0x00);
    writeData(0x33);
    writeData(0x33);

    writeReg(0xB7);
    writeData(0x35);

    writeReg(0xBB);
    writeData(0x1F);

    writeReg(0xC0);
    writeData(0x2C);

    writeReg(0xC2);
    writeData(0x01);
    writeData(0xC3);

    writeReg(0xC4);
    writeData(0x20);

    writeReg(0xC6);
    writeData(0x0F);

    writeReg(0xD0);
    writeData(0xA4);
    writeData(0xA1);

    writeReg(0xE0);
    writeData(0xD0);
    writeData(0x08);
    writeData(0x11);
    writeData(0x08);
    writeData(0x0C);
    writeData(0x15);
    writeData(0x39);
    writeData(0x33);
    writeData(0x50);
    writeData(0x36);
    writeData(0x13);
    writeData(0x14);
    writeData(0x29);
    writeData(0x2D);

    writeReg(0xE1);
    writeData(0xD0);
    writeData(0x08);
    writeData(0x10);
    writeData(0x08);
    writeData(0x06);
    writeData(0x06);
    writeData(0x39);
    writeData(0x44);
    writeData(0x51);
    writeData(0x0B);
    writeData(0x16);
    writeData(0x14);
    writeData(0x2F);
    writeData(0x31);
    writeData(0x2D);

    // Display on
    writeReg(0x29);
    writeReg(0x11);

    writeReg(0x35);
    writeData(0x00);

    //Fill display
    setTextColor(make_pair(white, black));
    clear(black);
}

/// The base address is 0x60000000 because the CS is set to FMC_NE1
DisplayImpl::DisplayMemLayout *const DisplayImpl::DISPLAY=
        reinterpret_cast<DisplayImpl::DisplayMemLayout*>(0x60000000);

} //namespace mxgui

#endif 