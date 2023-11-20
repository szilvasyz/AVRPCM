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

#define ANA_INPIN A0

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
int recNo = 1;


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

  Serial.println("Press to record");
  while (btn.get() == 0);

  sprintf(nBuf, "rec%05d.wav", recNo);
  Serial.print("File open: ");
  Serial.println(dataFile.open(nBuf, O_RDWR | O_CREAT));
  Serial.print("Recording file ");
  Serial.println(nBuf);

  recFile(&dataFile);
  dataFile.close();
  recNo++;
}

File32 *ff;

size_t readHandler(uint8_t *b, size_t s) {
  return ff->readBytes(b, s);
}

int wavInfo(File32 *f) {
  ff = f;
  return W.processBuffer(readHandler);
}


void recFile(File32 *f) {
  uint16_t sr = 16000;
  uint32_t ds = 0, ss = 0;

  if (f->isWritable()) {
    Serial.print("Setup: ");
    Serial.println(PCM_setupPWM(sr, 0));
    Serial.print("Start recording: ");
    Serial.println(PCM_startRec(true));

    while (btn.get() == 0) {
      f->write(PCM_getRecBuf(), PCM_BUFSIZ);
      PCM_releaseRecBuf();
      ss += PCM_BUFSIZ;
      if ((++ds % 10) == 0) {
        Serial.print(".");
        if (ds >= 400) {
          Serial.println("!");
          ds = 0;
        }
      }
    }
  }
  else
    Serial.println("File not available.");

  Serial.println("Done.");
  f->truncate(ss);

  Serial.print("Stop recording: ");
  Serial.println(PCM_stop());
  sprintf(sBuf, "%0ld bytes written.", ss);
  Serial.println(sBuf);

}
