#ifndef AVRPCM_H
#define AVRPCM_H


//#define PCM_USESDLIB 1

#define PCM_BUFSIZ 256
#define PCM_NUMBUF 3
#define PCM_NRMPCT 98
#define PCM_NRMSMP 10
#define PCM_AMPMAX 80

#define PCM_MAXVAL 127
#define PCM_OFFSET 128

#define PCM_SINE 1
#define PCM_SQUARE 2
#define PCM_SAWTOOTH 3
#define PCM_TRIANGLE 4

#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
  #warning "Using ATmega328 pins"
  #define PCM_ANAOUT 9
#elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || \
      defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
  #warning "Using ATmega644/1284 pins"
  #define PCM_ANAOUT 13
#else
  #error "Only ATmega328 and ATmega644/1284 supported."
#endif

#if __has_include("AVRPCM_config.h")
#include "AVRPCM_config.h"
#endif


#include "Arduino.h"
#ifdef PCM_USESDLIB
  #include "SD.h"
#endif

typedef void (PCM_ISRCallBack_t());


ISR(TIMER1_OVF_vect);
void PCM_ISRDummy();
void PCM_ISR();

int PCM_init(int digOutPin, int anaInPin = A0);
int PCM_setupPWM(uint16_t sampleRate, uint8_t invert);
int PCM_startPlay(uint8_t normalize);
uint8_t *PCM_getPlayBuf();
int PCM_pushPlayBuf();
int PCM_startRec(uint8_t normalize);
uint8_t *PCM_getRecBuf();
int PCM_releaseRecBuf();
int PCM_startGen(uint16_t frequency, uint8_t ampPercent, uint8_t waveForm);
int PCM_stop();
void PCM_clearOverrun();
uint32_t PCM_getOverrun();
void PCM_setPause(uint8_t p);
void PCM_setPlayInv(uint8_t inv);
void PCM_setRecInv(uint8_t inv);
uint8_t PCM_getPlayInv();
uint8_t PCM_getRecInv();


#ifdef PCM_USESDLIB
  int PCM_doSDPlay(File *dataFile);
#endif

#endif
