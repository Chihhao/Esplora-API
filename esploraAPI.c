/*
 * Author: Antonov Alexandr (Bismuth208)
 * e-mail: bismuth20883@gmail.com
 *
 *  THIS PROJECT IS PROVIDED FOR EDUCATION/HOBBY USE ONLY
 *  NO PROTION OF THIS WORK CAN BE USED IN COMMERIAL
 *  APPLICATION WITHOUT WRITTEN PERMISSION FROM THE AUTHOR
 *
 *  EVERYONE IS FREE TO POST/PUBLISH THIS ARTICLE IN
 *  PRINTED OR ELECTRONIC FORM IN FREE/PAID WEBSITES/MAGAZINES/BOOKS
 *  IF PROPER CREDIT TO ORIGINAL AUTHOR IS MENTIONED WITH LINKS TO
 *  ORIGINAL ARTICLE
 */

#include "esploraAPI.h"

// ------------------------------------ //
#define PASTE(x,y)  x ## y
#define PORT(x)     PASTE(PORT,x)
//#define PIN(x)      PASTE(PIN,x)
#define DDR(x)      PASTE(DDR,x)

// ------------------------------------ //
// defines for 74HC4067D muxtiplexer

#define MUXA_PIN      PF7
#define MUXB_PIN      PF6
#define MUXC_PIN      PF5
#define MUXD_PIN      PF4

#define MUX_READ_PIN  PF1

#define MUX_DDRX      DDRF
#define MUX_PORTX     PORTF

// ------------------------------------ //
// defines for RGB LED
#define LED_PIN_R     PC6
#define LED_PIN_B     PB5
#define LED_PIN_G     PB6

#define LED_PORTX_BG  PORTB
#define LED_PORTX_R   PORTC

#define LED_DDRX_R    DDRC
#define LED_DDRX_BG   DDRB

// ------------------------------------ //
// defines for Accelerometer
#define ACC_PIN_X      PF0
#define ACC_PIN_Y      PD6
#define ACC_PIN_Z      PD4

#define ACC_PORTX_X    PORTF
#define ACC_PORTX_YZ   PORTD

#define ACC_DDRX_X     DDRF
#define ACC_DDRX_YZ    DDRD

// ------------------------------------ //

#define SW_BTN_MIN_LVL 800
// ------------------------------------ //

btnStatus_t btnStates;
// ------------------------------------ //

__attribute__ ((optimize("O2"))) void initMuxIO(void)
{
  // setup multiplexor IO control as output
  MUX_DDRX  |= ((1<<MUXD_PIN) | (1<<MUXC_PIN) | (1<<MUXB_PIN) | (1<<MUXA_PIN));
  
  // set analoge input from MUX as input and pull-up
  MUX_DDRX &= ~(1<<MUX_READ_PIN);
  MUX_PORTX |= (1<<MUX_READ_PIN);
}

__attribute__ ((optimize("O2"))) void initLEDIO(void)
{
  // set as output
  LED_DDRX_BG |= (1<<LED_PIN_B) | (1<<LED_PIN_G);
  LED_DDRX_R  |= (1<<LED_PIN_R);
  // turn off
  LED_PORTX_R   &= ~(1<<LED_PIN_R);
  LED_PORTX_BG  &= ~((1<<LED_PIN_B) | (1<<LED_PIN_G));
}

__attribute__ ((optimize("O2"))) void initAccelerometerIO(void)
{
  // set as input
  ACC_DDRX_X  &= ~(1<<ACC_PIN_X);
  ACC_DDRX_YZ &= ~((1<<ACC_PIN_Y) | (1<<ACC_PIN_Z));
  
  // pull-up
  ACC_PORTX_X |= (1<<ACC_PIN_X);
  ACC_DDRX_YZ |= ((1<<ACC_PIN_Y) | (1<<ACC_PIN_Z));
}

__attribute__ ((optimize("O2"))) void initEsplora(void)
{
  // disable USB for 32u4
  USBCON = 0;
  
  // init Perephirial
  initSysTickTimer();
  initSPI();
  initADC();
  
  // init LCD
  //initR(INITR_BLACKTAB);
  //initR(INITR_GREENTAB);  
  initR(INITR_WHITETAB);   // add this line
  //initRBlack();
  
  // setup IO
  initMuxIO();
  initLEDIO();
  initAccelerometerIO();
  
  // presetup as most used
  setChannelADC(ANALOG_MUX_CH);
}

__attribute__ ((optimize("O2"))) void initRand(void)
{
  // yes, it real "random"!
  seedRndNum(readMic());
  seedRndNum(readTemp());
  seedRndNum(readLight());
}

__attribute__ ((optimize("O2"))) void initEsploraGame(void)
{
  initRand();
#if ADD_SOUND
  initSFX();
#endif
  initPalette();
}

// ------------------------------------ //
__attribute__ ((optimize("O2"))) void setLEDValue(uint8_t pin, uint8_t state)
{
  volatile uint8_t *pPort;
  volatile uint8_t pPin;
  
  switch(pin)
  {
    case LED_R: {
      pPort = &LED_PORTX_R;
      pPin = LED_PIN_R;
    } break;
    
    case LED_G: {
      pPort = &LED_PORTX_BG;
      pPin = LED_PIN_G;
    } break;
      
    case LED_B: {
      pPort = &LED_PORTX_BG;
      pPin = LED_PIN_B;
    } break;
  }
  
  if(state) {
    *pPort |= (1<<pPin);
  } else {
    *pPort &= ~(1<<pPin);
  }
}

// ------------------------------------ //
__attribute__ ((optimize("O2"))) uint16_t readAccelerometr(uint8_t axy)
{
  setChannelADC(axy);
  uint16_t val = readADC();
  
  setChannelADC(ANALOG_MUX_CH);
  return val;
}

// ---------- MUX --------- //
__attribute__ ((optimize("O2"))) uint16_t getAnalogMux(uint8_t chMux)
{
  MUX_PORTX = ((MUX_PORTX & 0x0F) | ((chMux<<4)&0xF0));
  
  return readADC();
}

__attribute__ ((optimize("O2"))) bool readSwitchButton(uint8_t btn)
{
  bool state = true;
  if(getAnalogMux(btn) > SW_BTN_MIN_LVL) { // state low == pressed
    state = false;
  }
  return state;
}

__attribute__ ((optimize("O2"))) uint16_t readJoystic(uint8_t btn)
{
  return getAnalogMux(btn);
}

__attribute__ ((optimize("O2"))) uint16_t readMic(void)
{
  return getAnalogMux(MIC_MUX_VAL);
}

__attribute__ ((optimize("O2"))) uint16_t readLight(void)
{
  return getAnalogMux(LIGHT_MUX_VAL);
}

__attribute__ ((optimize("O2"))) uint16_t readTemp(void)
{
  return getAnalogMux(TEMP_MUX_VAL);
}

__attribute__ ((optimize("O2"))) uint16_t readLinear(void)
{
  return getAnalogMux(LINEAR_MUX_VAL);
}

// ------------------------------------ //
// poll periodically buttons states
__attribute__ ((optimize("O2"))) void updateBtnStates(void)
{
  if(buttonIsPressed(BUTTON_A))
    btnStates.aBtn = true;
  if(buttonIsPressed(BUTTON_B))
    btnStates.bBtn = true;
  if(buttonIsPressed(BUTTON_X))
    btnStates.xBtn = true;
  if(buttonIsPressed(BUTTON_Y))
    btnStates.yBtn = true;
}

__attribute__ ((optimize("O2"))) bool getBtnState(uint8_t btn)
{
  bool state = false;

  switch(btn)
  {
   case BUTTON_A: {
    state = btnStates.aBtn;
   } break;

   case BUTTON_B: {
    state = btnStates.bBtn;
   } break;

   case BUTTON_X: {
    state = btnStates.xBtn;
   } break;

   case BUTTON_Y: {
    state = btnStates.yBtn;
   } break;
  }

  return state;
}

__attribute__ ((optimize("O2"))) void resetBtnStates(void)
{
  btnStates.zBtn = 0;
}

__attribute__ ((optimize("O2"))) uint8_t getJoyStickValue(uint8_t pin)
{
  uint16_t newValuePin = readJoystic(pin);

  // approximate newValuePin data, and encode to ASCII 
  // (just because it works and i left it as is)
  return ((newValuePin * 9) >> 10) + 48; // '>>10' same as '/1024'
}

// ------------------------------------ //
// This fuction must to be called 1/20s or every 50ms !
__attribute__ ((optimize("O2"))) void playMusic(void)
{
#if ADD_SOUND
  sfxUpdateAll(); // update sound each frame, as sound engine is frame based
#endif
}

// ------------------------------------ //
__attribute__ ((optimize("O2"))) void setMainFreq(uint8_t ps)
{
  // This function set prescaller,
  // which change main F_CPU freq
  // I have Atmega32U4 and F_CPU == 16 MHz!
  // at another freq will be another result!
  CLKPR = 0x80;  //enable change prascaller
  CLKPR = ps;    //change prescaller of F_CPU
}
