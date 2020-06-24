#pragma once

#include <stdint.h>

#define BLANK 0x00
#define DOT(x) x | 0b01 // Turn on the dot pin

#define LETTER_DOUBLE_QUOTE 0b01000100

#define LETTER_SINGLE_QUOTE 0b01000000

#define LETTER_HYPHEN 0b00000010
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

extern const unsigned char digits_c[];

extern const uint8_t c_letters[];
