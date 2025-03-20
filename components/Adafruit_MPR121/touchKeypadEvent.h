#ifndef KEYPAD_EVENT_H
#define KEYPAD_EVENT_H

#include <stdint.h>
#include <Adafruit_Keypad.h>

union touchKeypadEvent {
  struct {
    uint8_t KEY;   ///< 按键代码（0-23）
    uint8_t EVENT; ///< 事件类型（按下或释放）
  } bit;               ///< 位域格式
  uint16_t reg;        ///< 寄存器格式
};

#endif // KEYPAD_EVENT_H