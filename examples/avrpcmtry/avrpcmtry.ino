#define USE_SDFATLIB
//#define USE_SDLIB

#include "AVRPCM.h"
#include "mybutton.h"

#define DIG_OUTPIN  6

#define BT1_PIN 18
#define BT2_PIN 19

#define SDCS 4


MyButton btn;

#ifdef USE_SDLIB
#include <SPI.h>
#include <SD.h>
#endif

#ifdef USE_SDFATLIB
#include "SdFat.h"
#define SD_FAT_TYPE 0
#define SPI_CLOCK SD_SCK_MHZ(50)
#define SD_CONFIG SdSpiConfig(SDCS, SHARED_SPI, SPI_CLOCK)
SdFat sd;
#endif

File dataFile;


void setup() {

  btn.add(BT1_PIN);
  btn.add(BT2_PIN);
  btn.begin();

  Serial.begin(115200);
  Serial.println("Start");

#ifdef USE_SDLIB
  if (!SD.begin(SDCS)) {
    Serial.println("Card failed, or not present");
  }
  else {
    Serial.println("card initialized.");
  }
#endif

#ifdef USE_SDFATLIB
  if (!sd.begin(SD_CONFIG)) {
    Serial.println("Card failed, or not present");
  }
  else {
    Serial.println("card initialized.");
  }
#endif

  Serial.print("Init: ");
  Serial.println(PCM_init(DIG_OUTPIN));
  Serial.print("Setup: ");
  Serial.println(PCM_setupPWM(16000, 0));
  Serial.print("StartGen: ");
  Serial.println(PCM_startGen(1000, 80, PCM_SQUARE));
  delay(2000);
  Serial.print("Stop: ");
  Serial.println(PCM_stop());
}

void loop() {
  int b;
  
  while (btn.get() == 0);

#ifdef USE_SDLIB
  dataFile = SD.open("j2test.wav");
#endif
#ifdef USE_SDFATLIB
  dataFile.open("j2test.wav", O_RDONLY);
#endif

  Serial.print("Setup: ");
  Serial.println(PCM_setupPWM(48000, 0));
  Serial.print("Start play: ");
  Serial.println(PCM_startPlay(true));
  
//  do {
//    b = PCM_doSDPlay(&dataFile);
//  } while (b);

  while (dataFile.available()) {
    dataFile.read(PCM_getBuf(), PCM_BUFSIZ);
    PCM_pushBuf();
  }
  
  dataFile.close();
  Serial.print("Stop play: ");
  Serial.println(PCM_stop());

}
