#include "MPR121_Keypad.h"

MPR121_Keypad::MPR121_Keypad(uint8_t address0, uint8_t address1)
  : cap0(), cap1(),
    i2cAddress0(address0), i2cAddress1(address1),
    lastTouched0(0), currentTouched0(0),
    lastTouched1(0), currentTouched1(0),
    head(0), tail(0), count(0) {}

bool MPR121_Keypad::begin() {
  Wire.begin();

  // 初始化第一个 MPR121
  if (!cap0.begin(i2cAddress0)) {
    return false;
  }
  lastTouched0 = cap0.touched();

  // 初始化第二个 MPR121
  if (!cap1.begin(i2cAddress1)) {
    return false;
  }
  lastTouched1 = cap1.touched();

  return true;
}

void MPR121_Keypad::tick() {
  // 处理第一个 MPR121
  currentTouched0 = cap0.touched();

  for (uint8_t i = 0; i < 12; i++) {
    // 检测按下事件
    if ((currentTouched0 & _BV(i)) && !(lastTouched0 & _BV(i))) {
      uint8_t mappedKey = TOUCHPAD_MAP[i];
      if (mappedKey != 255) {
        touchKeypadEvent e;
        e.bit.KEY = mappedKey;
        e.bit.EVENT = KEY_JUST_PRESSED;
        addEvent(e);
      }
    }
    // 检测释放事件
    if (!(currentTouched0 & _BV(i)) && (lastTouched0 & _BV(i))) {
      uint8_t mappedKey = TOUCHPAD_MAP[i];
      if (mappedKey != 255) {
        touchKeypadEvent e;
        e.bit.KEY = mappedKey;
        e.bit.EVENT = KEY_JUST_RELEASED;
        addEvent(e);
      }
    }
  }

  lastTouched0 = currentTouched0;

  // 处理第二个 MPR121
  currentTouched1 = cap1.touched();

  for (uint8_t i = 0; i < 12; i++) {
    // 使用 touch_map[i + 8] 进行映射
    uint8_t mappedKey = TOUCHPAD_MAP[i + 8];
    
    // 检测按下事件
    if ((currentTouched1 & _BV(i)) && !(lastTouched1 & _BV(i))) {
      if (mappedKey != 255) {
        touchKeypadEvent e;
        e.bit.KEY = mappedKey;
        e.bit.EVENT = KEY_JUST_PRESSED;
        addEvent(e);
      }
    }
    // 检测释放事件
    if (!(currentTouched1 & _BV(i)) && (lastTouched1 & _BV(i))) {
      if (mappedKey != 255) {
        touchKeypadEvent e;
        e.bit.KEY = mappedKey;
        e.bit.EVENT = KEY_JUST_RELEASED;
        addEvent(e);
      }
    }
  }

  lastTouched1 = currentTouched1;
}

bool MPR121_Keypad::available() {
  return count > 0;
}

touchKeypadEvent MPR121_Keypad::read() {
  touchKeypadEvent e = buffer[tail];
  tail = (tail + 1) % BUFFER_SIZE;
  count--;
  return e;
}

void MPR121_Keypad::addEvent(touchKeypadEvent e) {
  if (count < BUFFER_SIZE) {
    buffer[head] = e;
    head = (head + 1) % BUFFER_SIZE;
    count++;
  } else {
    // 缓冲区满，覆盖最旧的事件
    buffer[head] = e;
    head = (head + 1) % BUFFER_SIZE;
    tail = (tail + 1) % BUFFER_SIZE;
  }
}
