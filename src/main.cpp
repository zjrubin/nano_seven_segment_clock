#include <Arduino.h>
#include "ds3231.h"

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
#define DOT(x) x | 0b01 // Turn on the dot pin

#define NUM_ELEMENTS(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define DELAY 1000

#define ONES(x) x % 10
#define TENS(x) (x / 10) % 10
#define HUNDREDS(x) (x / 100) % 10
#define THOUSANDS(x) (x / 1000) % 10

void updateShiftRegister(const int digit);
void shift_out_time(const RtcDateTime &time);
inline uint8_t convert_24_hour_to_12_hour(uint8_t hours);

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
  Serial.begin(BAUD_RATE); // open the serial port at 9600 bps:

  setup_rtc_ds3231();
}

void loop()
{
  RtcDateTime now = get_datetime();
  shift_out_time(now);
  delay(1000);
}

void shift_out_time(const RtcDateTime &time)
{
  uint8_t hours = convert_24_hour_to_12_hour(time.Hour());
  uint8_t minutes = time.Minute();
  uint8_t seconds = time.Second();

  digitalWrite(latchPin, LOW);

  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[TENS(hours)]);
  shiftOut(dataPin, clockPin, LSBFIRST, DOT(digits_c[ONES(hours)]));

  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[TENS(minutes)]);

#if NUM_DISPLAYS == 4
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[ONES(minutes)]);

#elif NUM_DISPLAYS == 6
  shiftOut(dataPin, clockPin, LSBFIRST, DOT(digits_c[ONES(minutes)]));

  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[TENS(seconds)]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[ONES(seconds)]);
#endif

  digitalWrite(latchPin, HIGH);
}

void updateShiftRegister(const int digit)
{
  byte ones = ONES(digit);
  byte tens = TENS(digit);
  byte hundreds = HUNDREDS(digit);
  byte thousands = THOUSANDS(digit);

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[thousands]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[hundreds]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[tens]);
  shiftOut(dataPin, clockPin, LSBFIRST, digits_c[ones]);
  digitalWrite(latchPin, HIGH);
}

inline uint8_t convert_24_hour_to_12_hour(uint8_t hours)
{
  hours = hours % 12;
  return hours == 0 ? 12 : hours;
}
