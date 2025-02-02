/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <avr/pgmspace.h>

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "systicktimer.h"
#include "spi.h"
#include "st7735.h"

//-------------------------------------------------------------------------------------------//

uint8_t tabcolor, colstart, rowstart;

int16_t _width  = ST7735_TFTHEIGHT_18;
int16_t _height = ST7735_TFTWIDTH;

//-------------------------------------------------------------------------------------------//

__attribute__ ((optimize("O2"))) void writeCommand(uint8_t c)
{
  ENABLE_CMD();
  sendData8_SPI1(c);
  RELEASE_TFT();
}

__attribute__ ((optimize("O2"))) void writeData(uint8_t c)
{
  ENABLE_DATA();
  sendData8_SPI1(c);
  RELEASE_TFT();
}

__attribute__ ((optimize("O2"))) void writeWordData(uint16_t c)
{
  ENABLE_DATA();
  sendData16_SPI1(c);
  RELEASE_TFT();
}

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in PROGMEM byte array.
__attribute__ ((optimize("O2"))) void commandList(const uint8_t *addr)
{  
  uint8_t  numCommands, numArgs;
  uint16_t ms;
  
  numCommands = pgm_read_byte(addr++);   // Number of commands to follow
  while(numCommands--) {                 // For each command...
    writeCommand(pgm_read_byte(addr++)); // Read, issue command
    numArgs  = pgm_read_byte(addr++);    // Number of args to follow
    ms       = numArgs & DELAY;          // If hibit set, delay follows args
    numArgs &= ~DELAY;                   // Mask out delay bit
    while(numArgs--) {                   // For each argument...
      writeData(pgm_read_byte(addr++));  // Read, issue argument
    }
    
    if(ms) {
      ms = pgm_read_byte(addr++); // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      _delayMS(ms);
    }
  }
}

__attribute__ ((optimize("O2"))) void initTFT_GPIO(void)
{
  SET_BIT(TFT_DDRX_CS, TFT_CS_PIN);
  SET_BIT(TFT_DDRX_DC, TFT_DC_PIN);
  SET_BIT(TFT_DDRX_RES, TFT_RES_PIN);
}

// Initialization code common to both 'B' and 'R' type displays
__attribute__ ((optimize("O2"))) void commonInit(const uint8_t *cmdList)
{
  colstart  = rowstart = 0; // May be overridden in init func  
  
  initTFT_GPIO();
  SET_TFT_RES_LOW;
  
#if TFT_CS_ALWAS_ACTIVE
  GRAB_TFT_CS;
#endif
  
  SET_TFT_CS_LOW;
  SET_TFT_DC_HI;
        
  // toggle RST low to reset
  SET_TFT_RES_HI;  _delayMS(10);
  SET_TFT_RES_LOW; _delayMS(10);
  SET_TFT_RES_HI;  _delayMS(10);

  commandList(cmdList);
}

// Initialization for ST7735B screens
__attribute__ ((optimize("O2"))) void initB(void)
{
  commonInit(Bcmd);
}

// Initialization for ST7735R screens (green or red tabs)
__attribute__ ((optimize("O2"))) void initR(uint8_t options)
{
  commonInit(Rcmd1);
  if(options == INITR_GREENTAB) {
    commandList(Rcmd2green);
    colstart = 2;
    rowstart = 1;	
  }
  else if(options == INITR_WHITETAB) {     // add this line
    commandList(Rcmd2red);                 // add this line
    colstart = 1;                          // add this line
    rowstart = 2;                          // add this line
	tftSetRotation(1);                     // add this line 
  }   
  else if(options == INITR_144GREENTAB) {
    //_height = ST7735_TFTHEIGHT_144;
    commandList(Rcmd2green144);
    colstart = 2;
    rowstart = 3;

  } else {
    // colstart, rowstart left at default '0' values
    commandList(Rcmd2red);
  }
  commandList(Rcmd3);

  // if black, change MADCTL color filter
  if (options == INITR_BLACKTAB) {
    writeCommand(ST7735_MADCTL);
    writeData(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
  }

  tabcolor = options;
}

__attribute__ ((optimize("O2"))) void initRBlack(void)
{
  commonInit(RcmdBlack);
  tabcolor = INITR_BLACKTAB;
}

__attribute__ ((optimize("O2"))) void initG(void)
{
  commonInit(Gcmd);
}

// blow your mind
__attribute__ ((optimize("O2"))) void tftSetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  ENABLE_CMD();             // grab TFT CS and writecommand:
  sendData8_SPI1(ST7735_CASET); // Column addr set
  
  SET_TFT_DC_HI;          // writeData:
  sendData32_SPI1(x0+colstart, x1+colstart); // XSTART, XEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RASET); // Row addr set
  
  SET_TFT_DC_HI;           // writeData:
  sendData32_SPI1(y0+rowstart, y1+rowstart);     // YSTART,YEND

  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RAMWR); // write to RAM
  
  SET_TFT_DC_HI;  // ready accept data
  
  //SET_TFT_CS_HI; // disable in other func
}

__attribute__ ((optimize("O2"))) void tftSetVAddrWindow(uint8_t x0, uint8_t y0, uint8_t y1)
{
  ENABLE_CMD();             // grab TFT CS and writecommand:
  sendData8_SPI1(ST7735_CASET); // Column addr set
  
  SET_TFT_DC_HI;          // writeData:
  sendData32_SPI1(x0+colstart, x0+colstart); // XSTART, XEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RASET); // Row addr set
  
  SET_TFT_DC_HI;           // writeData:
  sendData32_SPI1(y0+rowstart, y1+rowstart);     // YSTART,YEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RAMWR); // write to RAM
  
  SET_TFT_DC_HI;  // ready accept data
  
  //SET_TFT_CS_HI; // disable in other func
}

__attribute__ ((optimize("O2"))) void tftSetHAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1)
{
  ENABLE_CMD();             // grab TFT CS and writecommand:
  sendData8_SPI1(ST7735_CASET); // Column addr set
  
  SET_TFT_DC_HI;          // writeData:
  sendData32_SPI1(x0+colstart, x1+colstart); // XSTART, XEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RASET); // Row addr set
  
  SET_TFT_DC_HI;           // writeData:
  sendData32_SPI1(y0+rowstart, y0+rowstart);     // YSTART,YEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RAMWR); // write to RAM
  
  SET_TFT_DC_HI;  // ready accept data
  
  //SET_TFT_CS_HI; // disable in other func
}

__attribute__ ((optimize("O2"))) void tftSetAddrPixel(uint8_t x0, uint8_t y0)
{
  ENABLE_CMD();             // grab TFT CS and writecommand:
  sendData8_SPI1(ST7735_CASET); // Column addr set
  
  SET_TFT_DC_HI;          // writeData:
  sendData32_SPI1(x0+colstart, x0+colstart); // XSTART, XEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RASET); // Row addr set
  
  SET_TFT_DC_HI;           // writeData:
  sendData32_SPI1(y0+rowstart, y0+rowstart);     // YSTART,YEND
  
  SET_TFT_DC_LOW;               // writecommand:
  sendData8_SPI1(ST7735_RAMWR); // write to RAM
  
  SET_TFT_DC_HI;  // ready accept data
  
  //SET_TFT_CS_HI; // disable in other func
}

__attribute__ ((optimize("O2"))) void tftSetRotation(uint8_t m)
{
  writeCommand(ST7735_MADCTL);
  uint8_t rotation = m % 4; // can't be higher than 3
  
  switch (rotation) {
   case 0:
     if (tabcolor == INITR_BLACKTAB) {
       writeData(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
     } else {
       writeData(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
     }
     _width  = ST7735_TFTWIDTH;

     if (tabcolor == INITR_144GREENTAB) 
       _height = ST7735_TFTHEIGHT_144;
     else
       _height = ST7735_TFTHEIGHT_18;

     break;
   case 1:
     if (tabcolor == INITR_BLACKTAB) {
       writeData(MADCTL_MY | MADCTL_MV | MADCTL_RGB);
     } else {
       writeData(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
     }

     if (tabcolor == INITR_144GREENTAB) 
       _width = ST7735_TFTHEIGHT_144;
     else
       _width = ST7735_TFTHEIGHT_18;

     _height = ST7735_TFTWIDTH;
     break;
  case 2:
     if (tabcolor == INITR_BLACKTAB) {
       writeData(MADCTL_RGB);
     } else {
       writeData(MADCTL_BGR);
     }
     _width  = ST7735_TFTWIDTH;
     if (tabcolor == INITR_144GREENTAB) 
       _height = ST7735_TFTHEIGHT_144;
     else
       _height = ST7735_TFTHEIGHT_18;

    break;
   case 3:
     if (tabcolor == INITR_BLACKTAB) {
       writeData(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
     } else {
       writeData(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
     }
     if (tabcolor == INITR_144GREENTAB) 
       _width = ST7735_TFTHEIGHT_144;
     else
       _width = ST7735_TFTHEIGHT_18;

     _height = ST7735_TFTWIDTH;
     break;
  }
}

// how much to scroll
__attribute__ ((optimize("O2"))) void tftScrollAddress(uint8_t VSP)
{
  writeCommand(ST7735_VSCRSADD); // Vertical scrolling start address
  writeWordData(VSP);
}

// set scrollong zone
__attribute__ ((optimize("O2"))) void tftSetScrollArea(uint8_t TFA, uint8_t BFA)
{
  writeCommand(ST7735_VSCRDEF); // Vertical scroll definition
  writeWordData(TFA);
  writeWordData(ST7735_TFTWIDTH - TFA - BFA-1);
  writeWordData(BFA);
}

__attribute__ ((optimize("O2"))) void tftScroll(uint8_t lines, uint8_t yStart)
{
  for(uint8_t i = 0; i < lines; i++) {
    if ((yStart++) == (_height - TFT_BOT_FIXED_AREA)) yStart = TFT_TOP_FIXED_AREA;
    tftScrollAddress(yStart);
  }
}

__attribute__ ((optimize("O2"))) void tftScrollSmooth(uint8_t lines, uint8_t yStart, uint8_t wait)
{
  for(uint8_t i = 0; i < lines; i++) {
    if((yStart++) == (_height - TFT_BOT_FIXED_AREA)) yStart = TFT_TOP_FIXED_AREA;
    tftScrollAddress(yStart);
    _delayMS(wait);
  }
}

__attribute__ ((optimize("O2"))) void tftSetSleep(bool enable)
{
  if (enable) {
    writeCommand(ST7735_SLPIN);
    writeCommand(ST7735_DISPOFF);
  } else {
    writeCommand(ST7735_SLPOUT);
    writeCommand(ST7735_DISPON);
    _delayMS(5);
  }
}

__attribute__ ((optimize("O2"))) void tftSetIdleMode(bool mode)
{
  if (mode) {
    writeCommand(ST7735_IDLEON);
  } else {
    writeCommand(ST7735_IDLEOFF);
  }
}

__attribute__ ((optimize("O2"))) void tftSetDispBrightness(uint8_t brightness)
{
  //writeCommand(ST7735_WRDBR);
  writeWordData(brightness);
}

__attribute__ ((optimize("O2"))) void tftSetInvertion(bool i)
{
  writeCommand(i ? ST7735_INVON : ST7735_INVOFF);
}
