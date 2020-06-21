#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <avr/power.h>
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
#define DP 0x01         // Decimal point
#define DOT(x) x | 0b01 // Turn on the dot pin

#define NUM_ELEMENTS(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define DELAY 1000

#define ONES(x) x % 10
#define TENS(x) (x / 10) % 10
#define HUNDREDS(x) (x / 100) % 10
#define THOUSANDS(x) (x / 1000) % 10

void task_hello_message(void *pvParameters);
void task_display_date(void *pvParameters);
void task_display_cycle(void *pvParameters);
void shift_out_time(const RtcDateTime &time);
void shift_out_date(const RtcDateTime &time);
void shift_out_digit(uint8_t digit);
void shift_out_bytes(const uint8_t *byte_array, size_t len);
uint8_t convert_24_hour_to_12_hour(uint8_t hours);

constexpr unsigned char digits_c[] = {ZERO, ONE, TWO, THREE, FOUR,
                                      FIVE, SIX, SEVEN, EIGHT, NINE};

constexpr int c_data_pin = 8;
constexpr int c_latch_pin = 9;
constexpr int c_clock_pin = 10;

void setup()
{
    // Digital Input Disable on Analogue Pins
    // When this bit is written logic one, the digital input buffer on the corresponding ADC pin is disabled.
    // The corresponding PIN Register bit will always read as zero when this bit is set. When an
    // analogue signal is applied to the ADC7..0 pin and the digital input from this pin is not needed, this
    // bit should be written logic one to reduce power consumption in the digital input buffer.

    // #if defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) // Mega with 2560
    //     DIDR0 = 0xFF;
    //     DIDR2 = 0xFF;
    // #elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284PA__) // Goldilocks with 1284p
    //     DIDR0 = 0xFF;
    // #elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__)                                        // assume we're using an Arduino with 328p
    //     DIDR0 = 0x3F;
    // #elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)                                                                   // assume we're using an Arduino Leonardo with 32u4
    //     DIDR0 = 0xF3;
    //     DIDR2 = 0x3F;
    // #endif

    //     // Analogue Comparator Disable
    //     // When the ACD bit is written logic one, the power to the Analogue Comparator is switched off.
    //     // This bit can be set at any time to turn off the Analogue Comparator.
    //     // This will reduce power consumption in Active and Idle mode.
    //     // When changing the ACD bit, the Analogue Comparator Interrupt must be disabled by clearing the ACIE bit in ACSR.
    //     // Otherwise an interrupt can occur when the ACD bit is changed.
    //     ACSR &= ~_BV(ACIE);
    //     ACSR |= _BV(ACD);

    //     // CHOOSE ANY OF THESE <avr/power.h> MACROS THAT YOU NEED.
    //     // Any *_disable() macro can be reversed by the corresponding *_enable() macro.

    //     // Disable the Analog to Digital Converter module.
    //     power_adc_disable();

    //     // Disable the Serial Peripheral Interface module.
    //     power_spi_disable();

    //     // Disable the Two Wire Interface or I2C module.
    //     // power_twi_disable();

    //     // Disable the Timer 0 module. millis() will stop working.
    //     power_timer0_disable();

    //     // Disable the Timer 1 module.
    //     power_timer1_disable();

    //     // Disable the Timer 2 module. Used for RTC in Goldilocks 1284p devices.
    //     power_timer2_disable();

    // Now continue to initialise Tasks, and configure the Interfaces (that are not disabled).
    // And do any other setup that you need to do.

    pinMode(c_latch_pin, OUTPUT);
    pinMode(c_data_pin, OUTPUT);
    pinMode(c_clock_pin, OUTPUT);

    setup_rtc_ds3231();

    xTaskCreate(task_hello_message, "hello_message",
                75, NULL, 3, NULL);

    xTaskCreate(task_display_cycle, "display_cycle",
                75, NULL, 2, NULL);

    xTaskCreate(task_display_date, "display_date",
                75, NULL, 1, NULL);
}

void loop() // This is the idle task
{
    // Update the display to show the current time
    RtcDateTime now = get_datetime();
    shift_out_time(now);
}

void task_display_date(void *pvParameters)
{
    for (;;)
    {
        RtcDateTime now = get_datetime();
        shift_out_date(now);

        delay(5000); // Freeze the display

        // Wait for 5 minute (5 * 60 ticks if  #define portUSE_WDTO WDTO_1S)
        vTaskDelay(5 * 60);
    }
}

void task_display_cycle(void *pvParameters)
{
    for (;;)
    {
        for (int i = 0; i < 10; ++i)
        {
            shift_out_digit(i);
            delay(100); // Freeze the display
        }

        for (int i = 9; i >= 0; --i)
        {
            shift_out_digit(i);
            delay(100); // Freeze the display
        }

        delay(500);

        // Wait for 10 minutes
        vTaskDelay(10 * 60);
    }
}

void task_hello_message(void *pvParameters)
{
    constexpr uint8_t hello_bytes[] = {0b01101110, 0b10011110, 0b00011100, 0b00011100, ZERO, DP};
    for (;;)
    {
        shift_out_bytes(hello_bytes, NUM_ELEMENTS(hello_bytes));
        delay(5000);

        vTaskDelay(15 * 60);
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

void shift_out_date(const RtcDateTime &time)
{
    uint8_t month = time.Month();
    uint8_t day = time.Day();
    uint16_t year = time.Year();

    digitalWrite(c_latch_pin, LOW);

    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[TENS(month)]);
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, DOT(digits_c[ONES(month)]));

    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[TENS(day)]);

#if NUM_DISPLAYS == 4
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[ONES(day)]);

#elif NUM_DISPLAYS == 6
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, DOT(digits_c[ONES(day)]));

    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[TENS(year)]);
    shiftOut(c_data_pin, c_clock_pin, LSBFIRST, digits_c[ONES(year)]);
#endif

    digitalWrite(c_latch_pin, HIGH);
}

void shift_out_digit(uint8_t digit)
{
    digitalWrite(c_latch_pin, LOW);

    for (int i = 0; i < NUM_DISPLAYS; ++i)
    {
        shiftOut(c_data_pin, c_clock_pin, LSBFIRST, DOT(digits_c[digit]));
    }

    digitalWrite(c_latch_pin, HIGH);
}

void shift_out_bytes(const uint8_t *byte_array, size_t len)
{
    digitalWrite(c_latch_pin, LOW);

    for (size_t i = 0; i < len; ++i)
    {
        shiftOut(c_data_pin, c_clock_pin, LSBFIRST, byte_array[i]);
    }

    digitalWrite(c_latch_pin, HIGH);
}

uint8_t convert_24_hour_to_12_hour(uint8_t hours)
{
    hours = hours % 12;
    return hours == 0 ? 12 : hours;
}
