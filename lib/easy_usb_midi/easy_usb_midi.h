#ifndef EASY_USB_MIDI_H
#define EASY_USB_MIDI_H

#include <Arduino.h>
#include <USB.h>
#include <USBMIDI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// MIDI 事件数据结构
typedef struct __attribute__((packed)) {
    uint8_t cn : 4;         // Cable Number
    midi_code_index_number_t cin : 4;        // Code Index Number
    uint8_t ch : 4;         // 通道
    midi_code_index_number_t event : 4; // 事件类型
    uint8_t note;           // 音符号/指令
    uint8_t vol;            // 强度/音量/指令参数
} midi_event_packed_t;

class EASY_USB_MIDI {
public:
    EASY_USB_MIDI();
    ~EASY_USB_MIDI();

    void begin();                 // 初始化 USB MIDI
    void tick();                  // 应周期性调用，处理 USB MIDI 数据
    int available();              // 检查是否有可用的 MIDI 事件
    midi_event_packed_t read();   // 读取一个 MIDI 事件
    void clear();                 // 清空队列中的 MIDI 事件
    void onMidiEvent(void (*callback)(midi_event_packed_t)); // 回调函数

private:
    USBMIDI MIDI;
    QueueHandle_t _midiQueue;
    void (*_midiCallback)(midi_event_packed_t);
};

#endif