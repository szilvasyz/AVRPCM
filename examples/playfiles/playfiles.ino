#include "AVRPCM.h"
#include "mybutton.h"
#include "WAVhdr.h"
#include "SdFat.h"

#define SD_CS 4
#define SD_FAT_TYPE 0
#define SPI_CLOCK SD_SCK_MHZ(20)
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SPI_CLOCK)


#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
#define DIG_OUTPIN  6
#else
#define DIG_OUTPIN  14
#endif

#define BT1_PIN 18
#define BT2_PIN 19


#define INIT_TRY 5

MyButton btn(BT1_PIN, BT2_PIN);
SdFat sd;
File dataFile;
WAVhdr W;


int sdready = false;


void playfile() {
  uint16_t sr = 48000;
  uint32_t ds = 0, ss = 0;
  int pct;
  int pct0 = 0;

  dataFile.open("chaos.wav", O_RDONLY);

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
          Serial.print("%\r");
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

void setup() {
  int init_cnt = INIT_TRY;

  Serial.begin(115200);
  Serial.println("Start");

  btn.begin();

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  
  delay(1000);
  
  while (!(sdready = sd.begin(SD_CONFIG)) && init_cnt--) {
    Serial.print("#");
    digitalWrite(SD_CS, HIGH);
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
  Serial.print("Buttons: ");
  Serial.println(btn.count());
  
}

void loop() {
  int b;
  
  while (btn.peek() == 0);
  Serial.print(btn.get());
  Serial.print(' ');
  
}
