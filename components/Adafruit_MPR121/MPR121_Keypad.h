#ifndef MPR121_KEYPAD_H
#define MPR121_KEYPAD_H

#include <Adafruit_MPR121.h>
#include <Wire.h>
#include <Arduino.h>
#include "touchKeypadEvent.h"

#define BUFFER_SIZE 16

class MPR121_Keypad {
public:
  /**
   * @brief 构造函数
   * @param address0 I2C 地址 0，默认为 0x5A
   * @param address1 I2C 地址 1，默认为 0x5B
   */
  MPR121_Keypad(uint8_t address0 = 0x5A, uint8_t address1 = 0x5B);

  /**
   * @brief 初始化两个 MPR121 模块
   * @return 成功返回 true，失败返回 false
   */
  bool begin();

  /**
   * @brief 检测并处理触摸事件
   */
  void tick();

  /**
   * @brief 检查是否有可用的按键事件
   * @return 如果有事件返回 true，否则返回 false
   */
  bool available();

  /**
   * @brief 读取一个按键事件
   * @return 返回一个 touchKeypadEvent 结构体
   */
  touchKeypadEvent read();

private:
  Adafruit_MPR121 cap0;      ///< 第一个 MPR121 实例
  Adafruit_MPR121 cap1;      ///< 第二个 MPR121 实例
  uint8_t i2cAddress0;       ///< 第一个 MPR121 的 I2C 地址
  uint8_t i2cAddress1;       ///< 第二个 MPR121 的 I2C 地址
  uint16_t lastTouched0;     ///< 第一个 MPR121 上一次触摸状态
  uint16_t currentTouched0;  ///< 第一个 MPR121 当前触摸状态
  uint16_t lastTouched1;     ///< 第二个 MPR121 上一次触摸状态
  uint16_t currentTouched1;  ///< 第二个 MPR121 当前触摸状态

  // 按键映射表
  const uint8_t TOUCHPAD_MAP[24] = {
    255, 255, 255, 255,
    11, 3, 10, 2, 9, 1, 0, 8,
    15, 7, 14, 6, 5, 13, 4, 12,
    255, 255, 255, 255
  };

  // 环形缓冲区
  touchKeypadEvent buffer[BUFFER_SIZE];
  int head;  ///< 缓冲区头指针
  int tail;  ///< 缓冲区尾指针
  int count; ///< 当前缓冲区中的事件数量

  /**
   * @brief 将一个事件添加到缓冲区
   * @param e 要添加的 touchKeypadEvent
   */
  void addEvent(touchKeypadEvent e);
};

#endif // MPR121_KEYPAD_H
