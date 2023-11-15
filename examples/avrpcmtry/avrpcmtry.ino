#define USE_SDFATLIB
//#define USE_SDLIB

#include "AVRPCM.h"
#include "mybutton.h"
#include "WAVhdr.h"

#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define DIG_OUTPIN  6
#else
#define DIG_OUTPIN  14
#endif

#define BT1_PIN 18
#define BT2_PIN 19

#define SDCS 4

#define INIT_TRY 5

#define FILENAME "chaos22.wav"

MyButton btn;

#ifdef USE_SDLIB
#include <SPI.h>
#include <SD.h>
#define SDINIT SD.begin(SDCS)
#define DATAFILEOPEN dataFile = SD.open(FILENAME)
#endif

#ifdef USE_SDFATLIB
#include "SdFat.h"
#define SD_FAT_TYPE 0
#define SPI_CLOCK SD_SCK_MHZ(20)
#define SD_CONFIG SdSpiConfig(SDCS, DEDICATED_SPI, SPI_CLOCK)
#define SDINIT sd.begin(SD_CONFIG)
#define DATAFILEOPEN dataFile.open(FILENAME, O_RDONLY)
SdFat sd;
#endif

File dataFile;
WAVhdr W;

int sdready = false;


void setup() {
  int init_cnt = INIT_TRY;

  btn.add(BT1_PIN);
  btn.add(BT2_PIN);
  btn.begin();

  pinMode(SDCS, OUTPUT);
  digitalWrite(SDCS, HIGH);
  
  Serial.begin(115200);
  Serial.println("Start");

  delay(1000);
  
  while (!(sdready = SDINIT) && init_cnt--) {
    Serial.print("#");
    digitalWrite(SDCS, HIGH);
    delay(500);
  }

  if (!sdready) {
    Serial.println("Card failed, or not present");
  }
  else {
    Serial.println("card initialized.");
  }

  Serial.print("Init: ");
  Serial.println(PCM_init(DIG_OUTPIN));
  Serial.print("Setup: ");
  Serial.println(PCM_setupPWM(16000, 0));
  Serial.print("StartGen: ");
  Serial.println(PCM_startGen(1000, 80, PCM_SQUARE));
  delay(2000);
  Serial.print("Stop: ");
  Serial.println(PCM_stop());
  Serial.print("Card present: ");
  Serial.println(sdready);
}

void loop() {
  int b;
  uint16_t sr = 48000;
  uint32_t ds = 0, ss = 0;
  int pct;
  int pct0 = 0;
  
  while (btn.get() == 0);

  DATAFILEOPEN;

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

      Serial.print("Setup: ");
      Serial.println(PCM_setupPWM(sr, 0));
      Serial.print("Start play: ");
      Serial.println(PCM_startPlay(true));
      
    //  do {
    //    b = PCM_doSDPlay(&dataFile);
    //  } while (b);
    
      while (ss < ds) {
        dataFile.read(PCM_getBuf(), PCM_BUFSIZ);
        PCM_pushBuf();
        ss += PCM_BUFSIZ;
        pct = 100 * ss / ds;
        if (pct != pct0 && (pct % 10) == 0) {
          pct0 = pct;
          Serial.print(pct);
          Serial.println("%");
        }
      }
      
      dataFile.close();
    }
    else {
      Serial.println("unknown file");
    }
  }
  else
    Serial.println("File not available.");

  Serial.println("Done.");

  Serial.print("Stop play: ");
  Serial.println(PCM_stop());

}
