#ifndef LED_H_
#define LED_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// each LED has a brightness file
/// total number of LEDs is 36
/// the 1,2,3 represents the R,G,B of the first LED

/// @brief turn on all the LEDs
/// @return error code, 0 means success
int led_turn_on_all();

/// @brief turn off all the LEDs
/// @return error code, 0 means success
int led_turn_off_all();

/// @brief turn on the LED with the specified index
/// @param idx from 0 to 11
/// @param r red brightness from 0 to 255
/// @param g green brightness from 0 to 255
/// @param b blue brightness from 0 to 255
/// @return error code, 0 means success
int led_turn_on_idx(int idx, int r, int g, int b);

/// @brief turn on the LED with the specified index of the microphone
/// @param idx from 0 to 5(6 microphones: 0 means the No.6 microphone, 1 means
/// the No.1 microphone ... )
/// @param r
/// @param g
/// @param b
/// @return error code, 0 means success
int led_turn_on_mic_idx(int idx, int r, int g, int b);

#endif  // LED_H_