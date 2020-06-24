#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <avr/power.h>
#include <string.h>

#include "ds3231.h"

#define BLANK 0x00
#define DOT(x) x | 0b01 // Turn on the dot pin

#define DP 0x01 // Decimal point

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

#define LETTER_A 0b11101110
#define LETTER_B 0b11111110
#define LETTER_C 0b10011100
#define LETTER_D 0b11111000
#define LETTER_E 0b10011110
#define LETTER_F 0b10001110
#define LETTER_G 0b10111100
#define LETTER_H 0b01101110
#define LETTER_I 0b01100000
#define LETTER_J 0b01111000
#define LETTER_K 0b10101110
#define LETTER_L 0b00011100
#define LETTER_M 0b10101000
#define LETTER_N 0b11101100
#define LETTER_O 0b11111100
#define LETTER_P 0b11001110
#define LETTER_Q 0b11100110
#define LETTER_R 0b10001100
#define LETTER_S 0b10110110
#define LETTER_T 0b00011110
#define LETTER_U 0b01111100
#define LETTER_V 0b01110100
#define LETTER_W 0b01010100
#define LETTER_X 0b00100110
#define LETTER_Y 0b01110110
#define LETTER_Z 0b11011010

#define LETTER_a 0b11111010
#define LETTER_b 0b00111110
#define LETTER_c 0b00011010
#define LETTER_d 0b01111010
#define LETTER_e 0b11011110
#define LETTER_f 0b10001110
#define LETTER_g 0b11110110
#define LETTER_h 0b00101110
#define LETTER_i 0b00100000
#define LETTER_j 0b01110000
#define LETTER_k 0b10101110
#define LETTER_l 0b00001100
#define LETTER_m 0b10101000
#define LETTER_n 0b00101010
#define LETTER_o 0b00111010
#define LETTER_p 0b11001110
#define LETTER_q 0b11100110
#define LETTER_r 0b00001010
#define LETTER_s 0b10110110
#define LETTER_t 0b00011110
#define LETTER_u 0b00111000
#define LETTER_v 0b00111000
#define LETTER_w 0b01010100
#define LETTER_x 0b00100110
#define LETTER_y 0b01110110
#define LETTER_z 0b11011010

#define NUM_ELEMENTS(x) \
    ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

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
void scroll_text(const char *text, size_t len, size_t scroll_delay_ms);
void blank_display();
uint8_t convert_24_hour_to_12_hour(uint8_t hours);

constexpr unsigned char digits_c[] = {ZERO, ONE, TWO, THREE, FOUR,
                                      FIVE, SIX, SEVEN, EIGHT, NINE};

constexpr uint8_t c_letters[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, DP, 0x00, ZERO,
    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN,
    EIGHT, NINE, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, LETTER_A, LETTER_B, LETTER_C, LETTER_D, LETTER_E,
    LETTER_F, LETTER_G, LETTER_H, LETTER_I, LETTER_J, LETTER_K, LETTER_L,
    LETTER_M, LETTER_N, LETTER_O, LETTER_P, LETTER_Q, LETTER_R, LETTER_S,
    LETTER_T, LETTER_U, LETTER_V, LETTER_W, LETTER_X, LETTER_Y, LETTER_Z,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, LETTER_a,
    LETTER_b, LETTER_c, LETTER_d, LETTER_e, LETTER_f, LETTER_g, LETTER_h,
    LETTER_i, LETTER_j, LETTER_k, LETTER_l, LETTER_m, LETTER_n, LETTER_o,
    LETTER_p, LETTER_q, LETTER_r, LETTER_s, LETTER_t, LETTER_u, LETTER_v,
    LETTER_w, LETTER_x, LETTER_y, LETTER_z};

constexpr int c_data_pin = 8;
constexpr int c_latch_pin = 9;
constexpr int c_clock_pin = 10;

void setup()
{
    pinMode(c_latch_pin, OUTPUT);
    pinMode(c_data_pin, OUTPUT);
    pinMode(c_clock_pin, OUTPUT);

    setup_rtc_ds3231();

    blank_display();

    //   scroll_text(
    //       "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz",
    //       strlen(
    //           "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ
    //           abcdefghijklmnopqrstuvwxyz"),
    //       1000);
    //   delay(1000);

    xTaskCreate(task_hello_message, "hello_message", 100, NULL, 3, NULL);

    xTaskCreate(task_display_cycle, "display_cycle", 100, NULL, 2, NULL);

    xTaskCreate(task_display_date, "display_date", 100, NULL, 1, NULL);
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
            delay(250); // Freeze the display
        }

        for (int i = 9; i >= 0; --i)
        {
            shift_out_digit(i);
            delay(250); // Freeze the display
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

void scroll_text(const char *text, size_t len, size_t scroll_delay_ms)
{
    blank_display();

    for (size_t i = 0; i < len; ++i)
    {
        digitalWrite(c_latch_pin, LOW);

        shiftOut(c_data_pin, c_clock_pin, LSBFIRST,
                 c_letters[(size_t)text[i]]);

        digitalWrite(c_latch_pin, HIGH);

        delay(scroll_delay_ms);
    }
}

void blank_display()
{
    digitalWrite(c_latch_pin, LOW);

    for (size_t i = 0; i < NUM_DISPLAYS; ++i)
    {
        shiftOut(c_data_pin, c_clock_pin, LSBFIRST, BLANK);
    }

    digitalWrite(c_latch_pin, HIGH);
}

uint8_t convert_24_hour_to_12_hour(uint8_t hours)
{
    hours = hours % 12;
    return hours == 0 ? 12 : hours;
}
