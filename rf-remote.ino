#include <Arduino.h>
#include <VirtualWire.h>
#include <VirtualWire_Config.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#define TRX_PIN 3
#define RADIO_POWER_PIN 4
#define BUT1_PIN 1
#define BUT2_PIN 0

#define INT_PIN 2

#define CMD_POWER_ON  "ON"
#define CMD_POWER_OFF  "OF"
#define CMD_LIGHT_UP  "UP"
#define CMD_LIGHT_DOWN  "DN"

#define BAUD_RATE 500

const char *cmdPowerOn = CMD_POWER_ON;
const char *cmdPowerOff = CMD_POWER_OFF;
const char *cmdLightUp = CMD_LIGHT_UP;
const char *cmdLightDown = CMD_LIGHT_DOWN;

int8_t button = -1;
uint16_t timer = 0;

void setup() {
  vw_set_tx_pin(TRX_PIN);
  vw_setup(BAUD_RATE);

  pinMode(RADIO_POWER_PIN, OUTPUT);
  
  pinMode(INT_PIN, OUTPUT);
  pinMode(BUT1_PIN, INPUT_PULLUP);
  pinMode(BUT2_PIN, INPUT_PULLUP);

  ADCSRA &= ~(1<<ADEN);
  power_adc_disable();
  power_usi_disable();
  sei();
}

void custDelay(int msec) {
  for (; msec > 0; msec--) {
    delayMicroseconds(1000);
  }
}

void radioOn() {
  digitalWrite(RADIO_POWER_PIN, HIGH);
  custDelay(20);
}

void radioOff() {
  custDelay(3);
  digitalWrite(RADIO_POWER_PIN, LOW);
}

void sendCmd(char *cmd) {
  vw_send((uint8_t *)cmd, strlen(cmd));
  vw_wait_tx();
}

void pciSetup(byte pin) {
    PCMSK |= bit(pin);
    GIFR  |= bit(PCIF);
    GIMSK |= bit(PCIE);
}

void pciRemove(byte pin) {
    PCMSK &= ~(bit (pin));
}

ISR(PCINT0_vect) {
  pciRemove(BUT1_PIN);
  pciRemove(BUT2_PIN);
  sleep_disable(); //after wake up (on INT0)
  power_timer0_enable();
  power_timer1_enable();
}

void loop() {
  custDelay(10); //wait for evental button debounce
  if (digitalRead(BUT1_PIN) == LOW) {
    button = BUT1_PIN;
  } if (digitalRead(BUT2_PIN) == LOW) {
    button = BUT2_PIN;
  }

  if (button > -1) {
    radioOn();
    while(digitalRead(button) == LOW) {
      if (timer > 4) {
        if (button == BUT1_PIN) {
          sendCmd(cmdLightUp);
        } else {
          sendCmd(cmdLightDown);
        }
      }
      timer++;
      custDelay(50);
    }
    if (timer < 5) {
      if (button == BUT1_PIN) {
        sendCmd(cmdPowerOn);
      } else {
        sendCmd(cmdPowerOff);
      }
    }
    button = -1;
    timer = 0;
    radioOff();
  }
  // append Interrupts 
  pciSetup(BUT1_PIN);
  pciSetup(BUT2_PIN);
  // go to sleep
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  power_timer0_disable();
  power_timer1_disable();
  sleep_enable();
  sleep_bod_disable();
  sleep_cpu();
  //zzzz.....
  //next will be ISR(PCINT0_vect)
} 
