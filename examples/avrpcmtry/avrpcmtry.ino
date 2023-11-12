#define USE_SDFATLIB
//#define USE_SDLIB

#include "AVRPCM.h"
#include "mybutton.h"
#include "WAVhdr.h"

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
WAVhdr W;


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
  uint16_t sr = 48000;
  uint32_t ds = 0, ss = 0;
  int pct;
  
  while (btn.get() == 0);

#ifdef USE_SDLIB
  dataFile = SD.open("j2test.wav");
#endif
#ifdef USE_SDFATLIB
  dataFile.open("j2test.wav", O_RDONLY);
#endif

  if (dataFile.available()) {
    Serial.println();
    dataFile.read(W.getBuffer(), WAVHDR_LEN);
    if (W.processBuffer()) {
      Serial.print("samplerate=");
      Serial.println(W.getData().sampleRate);
      Serial.print("bitsPerSample=");
      Serial.println(W.getData().bitsPerSample);
      Serial.print("numChannels=");
      Serial.println(W.getData().numChannels);
      Serial.print("dataSize=");
      Serial.println(W.getData().dataSize);
      sr = W.getData().sampleRate;
      ds = W.getData().dataSize;
    }
    else {
      Serial.println("unknown file");
    }
  }
  Serial.print("Setup: ");
  Serial.println(PCM_setupPWM(sr, 0));
  Serial.print("Start play: ");
  Serial.println(PCM_startPlay(true));
  
//  do {
//    b = PCM_doSDPlay(&dataFile);
//  } while (b);

  while (dataFile.available()) {
    dataFile.read(PCM_getBuf(), PCM_BUFSIZ);
    PCM_pushBuf();
    ss += PCM_BUFSIZ;
    pct = 100 * ss / ds;
    Serial.print(pct);
    Serial.print("%\x0d");
  }
  
  dataFile.close();
  Serial.println("Done.");

  Serial.print("Stop play: ");
  Serial.println(PCM_stop());

}