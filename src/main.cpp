#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <string.h>

#include "characters.h"
#include "ds3231.h"

#define NUM_ELEMENTS(x) \
    ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define DELAY 1000

constexpr size_t c_minute_freertos = (60 * (1024 / portTICK_PERIOD_MS));
#define MINUTE_FREERTOS c_minute_freertos

#define ONES(x) x % 10
#define TENS(x) (x / 10) % 10
#define HUNDREDS(x) (x / 100) % 10
#define THOUSANDS(x) (x / 1000) % 10

void task_hello_message(void *pvParameters);
void task_display_messages(void *pvParameters);
void task_display_time(void *pvParameters);
void task_display_date(void *pvParameters);
void task_digit_cycle(void *pvParameters);
void display_date(const RtcDateTime &time);
void shift_out_time(const RtcDateTime &time);
void shift_out_date(const RtcDateTime &time);
void shift_out_digit(uint8_t digit);
void shift_out_bytes(const uint8_t *byte_array, size_t len);
void scroll_text(const char *text, size_t len, size_t scroll_delay_ms,
                 bool end_scroll_to_blank = false);
void blank_display();
uint8_t convert_24_hour_to_12_hour(uint8_t hours);

constexpr int c_data_pin = 8;
constexpr int c_latch_pin = 9;
constexpr int c_clock_pin = 10;

SemaphoreHandle_t g_rtc_mutex = NULL;

void setup()
{
    pinMode(c_latch_pin, OUTPUT);
    pinMode(c_data_pin, OUTPUT);
    pinMode(c_clock_pin, OUTPUT);

    setup_rtc_ds3231();

    blank_display();

    g_rtc_mutex = xSemaphoreCreateMutex();

    // xTaskCreate(task_hello_message, "hello_message", 125, NULL, 3, NULL);

    xTaskCreate(task_display_messages, "messages", 315, NULL, 3, NULL);

    xTaskCreate(task_digit_cycle, "display_cycle", 75, NULL, 2, NULL);

    xTaskCreate(task_display_date, "display_date", 100, NULL, 1, NULL);

    xTaskCreate(task_display_time, "display_time", 100, NULL, 0, NULL);
}

// This is the idle task
void loop()
{
}

void task_display_time(void *pvParameters)
{
    configASSERT(g_rtc_mutex != NULL);

    for (;;)
    {
        if (xSemaphoreTake(g_rtc_mutex, portMAX_DELAY) == pdTRUE)
        {
            RtcDateTime now = get_datetime();
            shift_out_time(now);
            xSemaphoreGive(g_rtc_mutex);
        }

        vTaskDelay(45 / portTICK_PERIOD_MS);
    }
}

void task_display_date(void *pvParameters)
{
    configASSERT(g_rtc_mutex != NULL);

    for (;;)
    {
        if (xSemaphoreTake(g_rtc_mutex, portMAX_DELAY) == pdTRUE)
        {
            RtcDateTime now = get_datetime();
            shift_out_date(now);
            xSemaphoreGive(g_rtc_mutex);
        }

        delay(5000); // Freeze the display

        vTaskDelay(3 * MINUTE_FREERTOS);
    }
}

void task_digit_cycle(void *pvParameters)
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

        vTaskDelay(10 * MINUTE_FREERTOS);
    }
}

void task_hello_message(void *pvParameters)
{
    constexpr uint8_t hello_bytes[] = {LETTER_H, LETTER_E, LETTER_L, LETTER_L, LETTER_O, DP};
    for (;;)
    {
        shift_out_bytes(hello_bytes, NUM_ELEMENTS(hello_bytes));
        delay(5000);

        vTaskDelay(15 * MINUTE_FREERTOS);
    }
}

void task_display_messages(void *pvParameters)
{
    static const char message_credits[] PROGMEM = "Seven-segment Clock by Zachary Rubin";
    static const char message_hello_world[] PROGMEM = "HELLO WORLD";
    static const char message_mending_wall[] PROGMEM = "He says again 'Good fences make good neighbors.'";
    static const char message_ozymandias[] PROGMEM = "My name is Ozymandias King of Kings. Look on my Works ye Mighty and despair";
    static const char message_hollow_men[] PROGMEM = "Mistah Kurtz-he dead.";
    static const char message_hamlet[] PROGMEM = "To be, or not to be that is the question Whether 'tis nobler in the mind to suffer The slings and arrows of outrageous fortune Or to take arms against a sea of troubles And by opposing end them.";
    static const char message_alphabet[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";
    static const char *const messages[] PROGMEM = {message_credits, message_alphabet, message_hello_world,
                                                   message_mending_wall, message_ozymandias,
                                                   message_hollow_men, message_hamlet};
    size_t i = 0;

    char message_buffer[200];

    for (;;)
    {
        // Read in message string from flash memory into SRAM
        strcpy_P(message_buffer, (char *)pgm_read_word(&(messages[i])));

        scroll_text(message_buffer, strlen(message_buffer), 400, true);
        delay(300);

        i = (i + 1) % NUM_ELEMENTS(messages);

        vTaskDelay(30 * MINUTE_FREERTOS);
    }
}

// void display_date(const RtcDateTime &time)
// {
//     shift_out_date(time);

//     delay(5000); // Freeze the display
// }

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

void scroll_text(const char *text, size_t len, size_t scroll_delay_ms, bool end_scroll_to_blank)
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

    if (end_scroll_to_blank)
    {
        // Blank the display by scrolling the text off of it
        for (size_t i = 0; i < NUM_DISPLAYS; ++i)
        {
            digitalWrite(c_latch_pin, LOW);

            shiftOut(c_data_pin, c_clock_pin, LSBFIRST, BLANK);

            digitalWrite(c_latch_pin, HIGH);

            delay(scroll_delay_ms);
        }
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
