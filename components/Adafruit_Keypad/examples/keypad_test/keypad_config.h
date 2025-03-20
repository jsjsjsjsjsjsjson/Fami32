// This file contains predefined setup for various Adafruit Matrix Keypads.
#ifndef __KEYPAD_CONFIG_H__
#define __KEYPAD_CONFIG_H__

#if defined(KEYPAD_PID3844)
const byte ROWS = 4; // rows
const byte COLS = 4; // columns
// define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {R1, R2, R3,
                      R4}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {C1, C2, C3,
                      C4}; // connect to the column pinouts of the keypad
#endif

#if defined(KEYPAD_PID1824) || defined(KEYPAD_PID3845) || defined(KEYPAD_PID419)
const byte ROWS = 4; // rows
const byte COLS = 3; // columns
// define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
    {'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};
byte rowPins[ROWS] = {R1, R2, R3,
                      R4};         // connect to the row pinouts of the keypad
byte colPins[COLS] = {C1, C2, C3}; // connect to the column pinouts of the
                                   // keypad
#endif

#if defined(KEYPAD_PID1332)
const byte ROWS = 1; // rows
const byte COLS = 4; // columns
// define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
    {'1', '2', '3', '4'},
};
byte rowPins[ROWS] = {R1}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {C1, C2, C3,
                      C4}; // connect to the column pinouts of the keypad
#endif

#endif