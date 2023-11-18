#include "AVRPCM.h"
#include "mybutton.h"
#include "WAVhdr.h"
#include "SdFat.h"

#define SD_CS 4
#define SD_FAT_TYPE 1
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
#define NAME_LEN 32
#define PRINT_LEN 60


MyButton btn(BT1_PIN, BT2_PIN);
SdFat sd;
WAVhdr W;


int sdready = false;
File32 dir;
File32 file;
File32 dataFile;
char nBuf[NAME_LEN] = "12345678ABC";
char sBuf[PRINT_LEN];


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
  
  if (!dir.open("/")) {
    Serial.println("Error opening root");
    while(true);
  }
}


void loop() {
  int b;
  
  if ((b = findNextWav()) == -1) {
    Serial.println("No more files");
    while (btn.get() == 0);
    dir.rewindDirectory();
    return;
  }

  dataFile.open(b, O_RDONLY);

  while (true) {

    if ((b = dataFile.getName(nBuf,20)) == 0)
      b = dataFile.getSFN(nBuf,20);

    sprintf(sBuf, "%3d %10ld %s ", dataFile.dirIndex(), dataFile.size(), nBuf);
    Serial.print(sBuf);
    wavInfo(&dataFile);
    sprintf(sBuf, "(sr:%u ch:%d bits:%d start:%lu)", 
      (uint16_t)W.getData().sampleRate,
      (uint16_t)W.getData().numChannels,
      (uint16_t)W.getData().bitsPerSample,
      (uint32_t)W.getData().dataPos);
    Serial.println(sBuf);

    while (true) {
      b = btn.get();
      if (b == 1) {
        playFile(&dataFile);
        break;
      }
      if (b == 2) {
        dataFile.close();
        return;
      }
    }
  }  
}

File32 *ff;

size_t readHandler(uint8_t *b, size_t s) {
  return ff->readBytes(b, s);
}

int wavInfo(File32 *f) {
  ff = f;
  return W.processBuffer(readHandler);
}


int findNextWav() {
  char b[13];
  int l;
  int i;

  while (file.openNext(&dir, O_RDONLY)) {
    l = file.getSFN(b, 13);
    i = file.dirIndex();
    file.close();
    delay(1);

    if ((strcasecmp("WAV", b + l - 3) == 0) && (b[0] != '_')) {
      return i;
    }
  }
  return -1;
}


void playFile(File32 *f) {
  uint16_t sr = 48000;
  uint32_t ds = 0, ss = 0;
  int pct;
  int pct0 = 0;

  f->rewind();
  if (f->available()) {
    if (wavInfo(f)) {
      sr = W.getData().sampleRate;
      ds = W.getData().dataSize;

      Serial.print("Setup: ");
      Serial.println(PCM_setupPWM(sr, 0));
      Serial.print("Start play: ");
      Serial.println(PCM_startPlay(true));
      
      while (ss < ds) {
        f->read(PCM_getBuf(), PCM_BUFSIZ);
        PCM_pushBuf();
        ss += PCM_BUFSIZ;
        pct = 100 * ss / ds;
        if (pct != pct0) {
          pct0 = pct;
          if (((pct % 10) % 3) == 1) Serial.print(".");
          if ((pct % 10) == 0) {
            Serial.print(pct);
            Serial.print("%");
          }
        }
        if (btn.get() != 0) {
          Serial.print(" -break-");
          break;
        }
      }
      Serial.println();
    }
    else {
      Serial.println("Invalid file.");
    }
  }
  else
    Serial.println("File not available.");

  Serial.println("Done.");

  Serial.print("Stop play: ");
  Serial.println(PCM_stop());
}
