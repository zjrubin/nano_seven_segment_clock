#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <avr/sleep.h>
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

void task_update_time(void *pvParameters);
void shift_out_time(const RtcDateTime &time);
uint8_t convert_24_hour_to_12_hour(uint8_t hours);

constexpr unsigned char digits_c[] = {ZERO, ONE, TWO, THREE, FOUR,
                                      FIVE, SIX, SEVEN, EIGHT, NINE};

constexpr int c_data_pin = 8;
constexpr int c_latch_pin = 9;
constexpr int c_clock_pin = 10;

void setup()
{
    xTaskCreate(task_update_time, "update_time",
                configMINIMAL_STACK_SIZE, NULL, 0, NULL);
}

void loop()
{
    // There are several macros provided in the  header file to actually put
    // the device into sleep mode.
    // See ATmega328p Datasheet for more detailed descriptions.
    // SLEEP_MODE_STANDBY
    // SLEEP_MODE_EXT_STANDBY

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    portENTER_CRITICAL();

    sleep_enable();

// Only if there is support to disable the brown-out detection.
// If the brown-out is not set, it doesn't cost much to check.
#if defined(BODS) && defined(BODSE)
    sleep_bod_disable();
#endif

    portEXIT_CRITICAL();

    sleep_cpu(); // Good night.

    // Ugh. Yawn... I've been woken up. Better disable sleep mode.
    // Reset the sleep_mode() faster than sleep_disable();
    sleep_reset();
}

void task_update_time(void *pvParameters)
{
    pinMode(c_latch_pin, OUTPUT);
    pinMode(c_data_pin, OUTPUT);
    pinMode(c_clock_pin, OUTPUT);

    setup_rtc_ds3231();

    for (;;)
    {
        RtcDateTime now = get_datetime();
        shift_out_time(now);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void shift_out_time(const RtcDateTime &time)
{
    uint8_t hours = convert_24_hour_to_12_hour(time.Hour());
    uint8_t minutes = time.Minute();
    uint8_t seconds = time.Second();
    digitalWrite(c_latch_pin, LOW);

    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[TENS(hours)]);
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, DOT(digits_c[ONES(hours)]));

    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[TENS(minutes)]);

#if NUM_DISPLAYS == 4
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[ONES(minutes)]);

#elif NUM_DISPLAYS == 6
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, DOT(digits_c[ONES(minutes)]));

    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[TENS(seconds)]);
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[ONES(seconds)]);
#endif

    digitalWrite(c_latch_pin, HIGH);
}

uint8_t convert_24_hour_to_12_hour(uint8_t hours)
{
    hours = hours % 12;
    return hours == 0 ? 12 : hours;
}
