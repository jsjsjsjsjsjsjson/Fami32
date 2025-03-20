#ifndef _ADAFRUIT_KEYPAD_H_
#define _ADAFRUIT_KEYPAD_H_

#include "Adafruit_Keypad_Ringbuffer.h"
#include "Arduino.h"
#include <string.h>

#define makeKeymap(x) ((byte *)x) ///< cast the passed key characters to bytes

#define KEY_JUST_RELEASED (0) ///< key has been released
#define KEY_JUST_PRESSED (1)  ///< key has been pressed

/**************************************************************************/
/*!
    @brief  key event structure
*/
/**************************************************************************/
union keypadEvent {
  struct {
    uint8_t KEY : 8;   ///< the keycode
    uint8_t EVENT : 8; ///< the edge
    uint8_t ROW : 8;   ///< the row number
    uint8_t COL : 8;   ///< the col number
  } bit;               ///< bitfield format
  uint32_t reg;        ///< register format
};

/**************************************************************************/
/*!
    @brief  Class for interfacing GPIO with a diode-multiplexed keypad
*/
/**************************************************************************/
class Adafruit_Keypad {
public:
  Adafruit_Keypad(byte *userKeymap, byte *row, byte *col, int numRows,
                  int numCols);
  ~Adafruit_Keypad();
  void begin();

  void tick();

  bool justPressed(byte key, bool clear = true);
  bool justReleased(byte key);
  bool isPressed(byte key);
  bool isReleased(byte key);
  int available();
  keypadEvent read();
  void clear();

private:
  byte *_userKeymap;
  byte *_row;
  byte *_col;
  volatile byte *_keystates;
  Adafruit_Keypad_Ringbuffer _eventbuf;

  int _numRows;
  int _numCols;

  volatile byte *getKeyState(byte key);
};

#endif
