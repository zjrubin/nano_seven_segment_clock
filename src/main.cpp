#include <Arduino.h>

#define ZERO 0xFC
#define ONE 0x60
#define TWO 0xDA
#define THREE 0xF2
#define FOUR 0x66
#define FIVE 0xB6
#define SIX 0xBE
#define SEVEN 0xE0
#define EIGHT 0xFE
#define NINE 0xE6

#define NUM_ELEMENTS(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define DELAY 1000

#define ONES(x) x % 10
#define TENS(x) (x / 10) % 10
#define HUNDREDS(x) (x / 100) % 10
#define THOUSANDS(x) (x / 1000) % 10

void updateShiftRegister(const int digit);

const unsigned char digits_c[] = {ZERO, ONE, TWO, THREE, FOUR,
                                  FIVE, SIX, SEVEN, EIGHT, NINE};

const int dataPin = 8;
const int latchPin = 9;
const int clockPin = 10;

void setup()
{
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  Serial.begin(9600); // open the serial port at 9600 bps:
}

void loop()
{
  for (int i = 0; i < 10000; i++)
  {
    updateShiftRegister(i);
    delay(DELAY);
  }
}

void updateShiftRegister(const int digit)
{
  byte ones = ONES(digit);
  byte tens = TENS(digit);
  byte hundreds = HUNDREDS(digit);
  byte thousands = THOUSANDS(digit);
  //Serial.println(tens);
  //Serial.println(ones);

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[thousands]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[hundreds]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[tens]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[ones]);
  digitalWrite(latchPin, HIGH);
}
