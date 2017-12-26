#include <Arduino.h>
#include <VirtualWire.h>
#include <VirtualWire_Config.h>
#include <avr/power.h>

#define RCV_PIN 2
#define LED_PIN 4

#define CMD_POWER_ON  "ON"
#define CMD_POWER_OFF  "OF"
#define CMD_LIGHT_UP  "UP"
#define CMD_LIGHT_DOWN  "DN"

#define CMD_LENGTH 8
#define BAUD_RATE 500

#define MAX_BRIGHT 32

uint8_t rcvBuf[CMD_LENGTH + 1];
uint8_t rcvBufLen = CMD_LENGTH;

int brightness = 15;
int saved = brightness;
int temp = saved;

#define FADE_TIME 1800
int fade = 0;

const uint8_t brightMap[] = {
    0,
    1,   2,   3,   4,   5,   6,   7,   8,
    9,  10,  12,  14,  16,  19,  22,  25,
   29,  34,  39,  45,  52,  60,  70,  81,
   93, 108, 125, 144, 166, 192, 222, 255
};

void custDelay(int msec) {
  for (; msec > 0; msec--) {
    delayMicroseconds(1000);
  }
}

void setLight(uint8_t b) {
  if (b > MAX_BRIGHT) { b = MAX_BRIGHT; }
  int8_t pwm = brightMap[b];
  OCR1B = pwm;
}

void turnOn() {
  for (int i = 1; i < brightness; i++) {
    setLight(i);
    custDelay(10);
  }
  setLight(brightness);
}

void turnOff() {
  for (int i = brightness; i >= 0; i--) {
    setLight(i);
    custDelay(10);
  }
  setLight(0);
  brightness = 0;
}

void blinken() {
  temp = brightness;
  turnOff();
  brightness = temp;
  turnOn();
}



void setup() {
//    Serial.begin(9600);  // Debugging only
//    Serial.println("setup");

  pinMode(RCV_PIN, INPUT);
  vw_set_rx_pin(RCV_PIN);
  vw_set_rx_inverted(0);
  vw_setup(BAUD_RATE);   // Bits per sec
  vw_rx_start();       // Start the receiver PLL running
  pinMode(LED_PIN, OUTPUT);

  ADCSRA &= ~(1<<ADEN);
  power_adc_disable();
  power_usi_disable();

  //set timer1 (PWM) clock prescaler
  TCCR1 = (TCCR1 & 0b11110000) | (0b0001); //PCK
  GTCCR |= _BV(COM1B1); //set up pin 4 for PWM
  GTCCR &= ~_BV(COM1B0);  

  sei();

  turnOn();
  blinken();
  turnOff();
}


void loop() {
//  Serial.println("waiting...");
//  vw_wait_rx();
  rcvBufLen = CMD_LENGTH;
  if (vw_get_message(rcvBuf, &rcvBufLen)) { // Non-blocking
//    Serial.print("Got: ");
    rcvBuf[rcvBufLen] = 0; //null terminated, we have one xtra byte allocated for this...
    String cmd = String((char *)rcvBuf);
//    Serial.println(cmd);
    if (cmd == CMD_POWER_ON) {
      if (brightness == 0) { //turn on only
        brightness = saved;
        turnOn();
      } else if(fade == 0) { //set timer to fade in FADE_TIME cycles
        //blink for acknowledge
        blinken();
        //set fade to 
        fade = FADE_TIME;
        saved = brightness;
      }
    } else if ((cmd == CMD_POWER_OFF) && (brightness > 0)) {
      saved = brightness;
      turnOff();
      fade = 0;
    } else if ((cmd == CMD_LIGHT_UP) && (brightness > 0) && (brightness < (MAX_BRIGHT - 1))) {
      brightness++;
    } else if ((cmd == CMD_LIGHT_DOWN) && (brightness > 1)) {
      brightness--;
    }
  }

  if (fade > 0) { //fade timer is set up
    if (fade == 1) {
      turnOff();
    } else if (fade <= (saved * 16)) {
      brightness = fade / 16;
    }
    fade--;
  }

  setLight(brightness);
  custDelay(100);
}
