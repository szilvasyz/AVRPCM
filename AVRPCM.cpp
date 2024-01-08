#include "AVRPCM.h"


// ports
int PCM_digOutPin;
int PCM_anaOutPin;
volatile uint8_t *PCM_digOutPort;
uint8_t PCM_digOutMask;
uint8_t PCM_digOutInvMask;
uint8_t PCM_digOutHi = 0;
uint8_t PCM_digOutLo = 0;
uint8_t PCM_anaInChannel;

#ifdef PCM_IRQDEBUG
  volatile uint8_t *PCM_debugPort;
  uint8_t PCM_debugMask;
  uint8_t PCM_debugInvMask;
#endif

// PWM generation
uint16_t PCM_pwmMul = 1;
uint16_t PCM_pwmOfs = 128;
uint8_t PCM_playInv = 0;
uint8_t PCM_recInv = 0;
uint16_t PCM_pwmTop;
uint16_t PCM_sampleRate;
uint8_t PCM_downSample = 0;


// PWM amplifying/normalizing
uint16_t PCM_ampMul = 4;
uint16_t PCM_ampOfs = 128;
uint16_t PCM_ampNrm = 120;
uint8_t PCM_ampCnt;
uint8_t PCM_normalize;

// data buffers
volatile uint8_t PCM_dataBuf[PCM_NUMBUF][PCM_BUFSIZ];
volatile uint8_t PCM_busyBuf[PCM_NUMBUF];
volatile uint16_t PCM_bufSel = 0;
volatile uint16_t PCM_bufPtr = 0;
volatile uint16_t PCM_bufWrt = 0;
volatile uint32_t PCM_overrun = 0;

// waveform generating
uint16_t PCM_fStep;
uint16_t PCM_fSMax;
uint16_t PCM_fShft;

// modes
uint8_t PCM_playing = false;
uint8_t PCM_recording = false;
uint8_t PCM_generating = false;
uint8_t PCM_paused = false;
uint8_t PCM_running = false;



PCM_ISRCallBack_t *PCM_OVFHandler = PCM_ISRDummy;


ISR(TIMER1_OVF_vect) {
  PCM_OVFHandler();
}


void PCM_ISRDummy() {}


void PCM_ISR() {
  static uint8_t pwmCnt = 1;
  static uint16_t fRatio = 0;
  static uint16_t fSNum = 1;
  static uint8_t pcmflip = 0;

  static uint16_t d;

  #ifdef PCM_IRQDEBUG
    *PCM_debugPort &= PCM_debugInvMask;
  #endif

  if (PCM_paused) {
    #ifdef PCM_IRQDEBUG
      *PCM_debugPort |= PCM_debugMask;
    #endif
    return;
  }

  pcmflip = !pcmflip;

  if (--pwmCnt != 0) {
    #ifdef PCM_IRQDEBUG
      *PCM_debugPort |= PCM_debugMask;
    #endif
    return;
  }
  pwmCnt = PCM_pwmMul;
  d = PCM_pwmOfs;

  if (PCM_generating) {
    fRatio += PCM_fStep;
    if (--fSNum == 0) {
      fRatio = 0;
      fSNum = PCM_fSMax;
    }
    d = ((PCM_ampMul * PCM_dataBuf[0][fRatio >> PCM_fShft]) >> 2) - PCM_ampOfs;
  }

  if (PCM_playing) {
    if (PCM_busyBuf[PCM_bufSel] == 1) {
      d = PCM_dataBuf[PCM_bufSel][PCM_bufPtr++];
      if (PCM_downSample) {
        d = (d + PCM_dataBuf[PCM_bufSel][PCM_bufPtr++]) >> 1;
        //PCM_bufPtr++;
      }
      d = ((PCM_ampMul * d) >> 2) - PCM_ampOfs;
      if (PCM_bufPtr == PCM_BUFSIZ) {
        PCM_bufPtr = 0;
        PCM_busyBuf[PCM_bufSel++] = 0;
        if (PCM_bufSel == PCM_NUMBUF) PCM_bufSel = 0;
      }
    }
    else
      PCM_overrun++;
  }

  if (PCM_recording) {
    if (PCM_busyBuf[PCM_bufSel] == 0) {
      d = ADCH;
      if (PCM_recInv) {
        d = (~d) & 0xFF; 
      }
      PCM_dataBuf[PCM_bufSel][PCM_bufPtr] = d;
      if (++PCM_bufPtr == PCM_BUFSIZ) {
        PCM_bufPtr = 0;
        PCM_busyBuf[PCM_bufSel] = 1;
        if (++PCM_bufSel == PCM_NUMBUF) PCM_bufSel = 0;
      }
    }
    else
      PCM_overrun++;
  }

  if (PCM_generating || PCM_playing) {
  
    OCR1A = d;

    if (d > PCM_pwmOfs)
      *PCM_digOutPort = *PCM_digOutPort & PCM_digOutInvMask | PCM_digOutHi;
    else
      *PCM_digOutPort = *PCM_digOutPort & PCM_digOutInvMask | PCM_digOutLo;

  }

  #ifdef PCM_IRQDEBUG
    *PCM_debugPort |= PCM_debugMask;
  #endif

}


int PCM_init(int digOutPin, int anaInPin = A0) {

  PCM_anaOutPin  = PCM_ANAOUT;
  pinMode(PCM_ANAOUT, OUTPUT);

  PCM_digOutPin  = digOutPin;
  pinMode(digOutPin, OUTPUT);

  PCM_anaInChannel = analogPinToChannel(anaInPin);
  pinMode(anaInPin, INPUT);
  DIDR0 |= _BV(PCM_anaInChannel);
  ADCSRB = 0;
  ADMUX  = PCM_anaInChannel | _BV(ADLAR);
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS0);

  #ifdef PCM_IRQDEBUG
    pinMode(PCM_IRQDEBUG, OUTPUT);
  #endif

  // for fast register-level access
  PCM_digOutPort = portOutputRegister(digitalPinToPort(digOutPin));
  PCM_digOutMask = digitalPinToBitMask(digOutPin);
  PCM_digOutInvMask = ~PCM_digOutMask;
  PCM_digOutHi = PCM_digOutMask;
  PCM_digOutLo = 0;

  #ifdef PCM_IRQDEBUG
    PCM_debugPort = portOutputRegister(digitalPinToPort(PCM_IRQDEBUG));
    PCM_debugMask = digitalPinToBitMask(PCM_IRQDEBUG);
    PCM_debugInvMask = ~digitalPinToBitMask(PCM_IRQDEBUG);
  #endif

  return true;
}


int PCM_setupPWM(uint16_t sampleRate, uint8_t invert) {
  uint16_t pwmFrq;

  PCM_playing = false;
  PCM_recording = false;
  PCM_generating = false;

  PCM_sampleRate = sampleRate;

  if (PCM_sampleRate > 32000) {
    PCM_downSample = 1;
    PCM_sampleRate /= 2;
  }
  else
    PCM_downSample = 0;

  PCM_playInv = invert;
  PCM_recInv = invert;
  
  PCM_pwmMul = 32000 / PCM_sampleRate;
  if (PCM_pwmMul == 0) PCM_pwmMul = 1;
  pwmFrq = PCM_pwmMul * PCM_sampleRate;
  if (pwmFrq > F_CPU / 256)
    return false;

  PCM_pwmTop = F_CPU / pwmFrq - 1;
  PCM_pwmOfs = (PCM_pwmTop + 1) >> 1;
  PCM_ampNrm = (long)(PCM_pwmTop + 1) * PCM_NRMPCT / 200;

  #ifdef DEBUGPCM
    Serial.print("PCM_sampleRate:"); Serial.println(PCM_sampleRate);
    Serial.print("PCM_downSample:"); Serial.println(PCM_downSample);
    Serial.print("PCM_pwmMul    :"); Serial.println(PCM_pwmMul);
    Serial.print("pwmFrq        :"); Serial.println(pwmFrq);
    Serial.print("PCM_pwmTop    :"); Serial.println(PCM_pwmTop);
    Serial.print("PCM_pwmOfs    :"); Serial.println(PCM_pwmOfs);
    Serial.print("PCM_ampNrm    :"); Serial.println(PCM_ampNrm);
  #endif

  // TIMER1 setup: fast PWM on OCR1A
  // set TOP in ICR1
  // WGM1 = 1110
  ICR1 = PCM_pwmTop;
  if (invert) {
    TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11);
  }
  else {
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
  }

  TCCR1B = _BV(CS10) | _BV(WGM13) | _BV(WGM12);
  TCCR1C = 0;
  OCR1A = PCM_pwmOfs;
  TIMSK1 = _BV(TOIE1);

  PCM_ampMul = PCM_pwmTop * 2 / PCM_MAXVAL;
  PCM_ampOfs = ((PCM_ampMul * PCM_OFFSET) >> 2) - PCM_pwmOfs;

  PCM_OVFHandler = PCM_ISR;
  PCM_running = true;
  PCM_paused = true;
  return true;
}


int PCM_startPlay(uint8_t normalize) {
  uint16_t b,c;

  if (!PCM_running) return false;

  // amppwm = MAXAMP;
  // ampofs = (4 - amppwm) * (OFFSET >> 2) + pwmofs - OFFSET;
  // ampcnt = NRMSMP;
  for (b = 0; b < PCM_NUMBUF; b++) {
    for (c = 0; c < PCM_BUFSIZ; c++)
      PCM_dataBuf[b][c] = PCM_OFFSET;
    PCM_busyBuf[b] = 1;
  }
  PCM_bufSel = 0;
  PCM_bufWrt = 0;
  PCM_bufPtr = 0;
  PCM_normalize = normalize;

  if (normalize) {
    PCM_ampMul = PCM_AMPMAX;
    PCM_ampOfs = ((PCM_ampMul * PCM_OFFSET) >> 2) - PCM_pwmOfs;
    PCM_ampCnt = PCM_NRMSMP;
  }

  PCM_overrun = 0;
  PCM_paused = 0;
  PCM_playing = 1;
  return true;
}


uint8_t *PCM_getPlayBuf() {
  while (PCM_playing && PCM_busyBuf[PCM_bufWrt]);
  return (uint8_t *)PCM_dataBuf[PCM_bufWrt];
}


int PCM_pushPlayBuf() {
  int16_t a;
  uint16_t c;

  if (!PCM_playing) return false;

  PCM_busyBuf[PCM_bufWrt] = 1;
  if (++PCM_bufWrt == PCM_NUMBUF) PCM_bufWrt = 0;

  if (!PCM_normalize) return true;
  for (c = 0; c < PCM_BUFSIZ; c++) {
    a = PCM_dataBuf[PCM_bufWrt][c] - PCM_OFFSET;
    if ((((abs(a) * PCM_ampMul) >> 2) > PCM_ampNrm) && (PCM_ampMul > 4)) {
      if (PCM_ampCnt-- == 0) {
        PCM_ampCnt = PCM_NRMSMP;
        PCM_ampMul--;
        // Serial.println(PCM_ampMul);
        PCM_ampOfs -= (PCM_OFFSET >> 2);
      }
    }
  }

  return true;
}


int PCM_startRec(uint8_t normalize) {
  uint16_t b,c;

  if (!PCM_running) return false;

  for (b = 0; b < PCM_NUMBUF; b++) {
    for (c = 0; c < PCM_BUFSIZ; c++)
      PCM_dataBuf[b][c] = PCM_OFFSET;
    PCM_busyBuf[b] = 0;
  }
  PCM_bufSel = 0;
  PCM_bufWrt = 0;
  PCM_bufPtr = 0;
  PCM_normalize = normalize;

  if (normalize) {
    PCM_ampMul = PCM_AMPMAX;
    PCM_ampOfs = ((PCM_ampMul * PCM_OFFSET) >> 2) - PCM_pwmOfs;
    PCM_ampCnt = PCM_NRMSMP;
  }

  PCM_overrun = 0;
  PCM_paused = 0;
  PCM_recording = 1;
  return true;
}


uint8_t *PCM_getRecBuf() {
  while (PCM_recording && PCM_busyBuf[PCM_bufWrt] == 0);
  return (uint8_t *)PCM_dataBuf[PCM_bufWrt];
}


int PCM_releaseRecBuf() {
  if (!PCM_recording) return false;

  PCM_busyBuf[PCM_bufWrt] = 0;
  if (++PCM_bufWrt == PCM_NUMBUF) PCM_bufWrt = 0;

  return true;
}


int PCM_startGen(uint16_t frequency, uint8_t ampPercent, uint8_t waveForm) {
  int c;
  uint16_t bufSize;
  float d;

  if (!PCM_running) return false;

  // char cbuf[60];
  // sprintf(cbuf, "oldfreq f=%d, step=%d, smax=%d\n", genfreq, freqstep, freqsmax);
  // Serial.print(cbuf);
  PCM_fStep = ((long)frequency << 16) / PCM_sampleRate;
  PCM_fSMax = 0x10000L / PCM_fStep;

  // float f = samplerate / freqsmax;
  // sprintf(cbuf, "setfreq genfreq=%d, freqstep=%d, freqsmax=%d, realfreq=%d\n", genfreq, freqstep, freqsmax, int(f));
  // Serial.print(cbuf);

  c = PCM_BUFSIZ * PCM_NUMBUF;
  bufSize = 1;
  PCM_fShft = 16;
  while (c = (c >> 1)) {
    bufSize <<= 1;
    PCM_fShft--;
  }

  // sprintf(cbuf, "fillsamples freqsize=%d, freqshft=%d\n", freqsize, freqshft);
  // Serial.print(cbuf);

  for (c = 0; c < bufSize; c++) {
    switch (waveForm) {
      case PCM_SINE:
        d = (sin(2.0 * PI * c / bufSize));
        break;
      case PCM_SQUARE:
        d = (c < bufSize / 2 ? - 1 : 1);
        break;
      case PCM_SAWTOOTH:
        d = (2.0 * (float)c / bufSize - 1.0);
        break;
      case PCM_TRIANGLE:
        d = ((c < (bufSize / 2) ? 2 * c : bufSize - 2 * (c - bufSize / 2)) / (float)(bufSize / 2) - 1.0);
        break;
      default:
        return false;
    }
    PCM_dataBuf[0][c] = PCM_OFFSET + PCM_MAXVAL * ampPercent * d / 100;

  }

  PCM_generating = 1;
  return true;
}


int PCM_stop() {
  PCM_generating = 0;
  PCM_playing = 0;
  PCM_recording = 0;
  PCM_OVFHandler = PCM_ISRDummy;
  PCM_running = false;
  return true;
}


void PCM_clearOverrun() {
  PCM_overrun = 0;
}


uint32_t PCM_getOverrun() {
  return PCM_overrun;
}


void PCM_setPause(uint8_t p) {
  PCM_paused = p;
}


void PCM_setPlayInv(uint8_t inv) {
  PCM_playInv = inv;

  if (!PCM_running) return;

  if (inv) {
    TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11);
    PCM_digOutHi = 0;
    PCM_digOutLo = PCM_digOutMask;
  }
  else {
    TCCR1A = _BV(COM1A1) | _BV(WGM11);
    PCM_digOutLo = 0;
    PCM_digOutHi = PCM_digOutMask;
  }

}


void PCM_setRecInv(uint8_t inv) {
  PCM_recInv = inv;
}


uint8_t PCM_getPlayInv() {
  return PCM_playInv;
}


uint8_t PCM_getRecInv() {
  return PCM_recInv;
}


#ifdef PCM_USESDLIB
  int PCM_doSDPlay(File *dataFile) {

    if (PCM_generating || !PCM_running) return false;

    while (PCM_busyBuf[PCM_bufWrt] == 1);
    if (dataFile->available()) {
      dataFile->read((void *)(PCM_dataBuf[PCM_bufWrt]), PCM_BUFSIZ);
      PCM_pushBuf();
      return true;
    }
    return false;
  }
#endif
