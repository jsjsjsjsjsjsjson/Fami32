#include "MPR121_Keypad.h"

#include "fami32_pin.h"
#include "driver/i2c.h"
#include "esp_log.h"

namespace {
constexpr const char *TAG = "MPR121_Keypad";
constexpr i2c_port_t I2C_PORT = I2C_NUM_0;
constexpr uint32_t I2C_FREQ_HZ = 400000;
constexpr TickType_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(20);

constexpr uint8_t REG_TOUCH_STATUS_L = 0x00;
constexpr uint8_t REG_ECR = 0x5E;
constexpr uint8_t REG_SOFTRESET = 0x80;
constexpr uint8_t REG_CONFIG1 = 0x5C;
constexpr uint8_t REG_CONFIG2 = 0x5D;
constexpr uint8_t REG_DEBOUNCE = 0x5B;
constexpr uint8_t REG_TOUCHTH_0 = 0x41;
constexpr uint8_t REG_RELEASETH_0 = 0x42;

constexpr uint8_t REG_MHDR = 0x2B;
constexpr uint8_t REG_NHDR = 0x2C;
constexpr uint8_t REG_NCLR = 0x2D;
constexpr uint8_t REG_FDLR = 0x2E;
constexpr uint8_t REG_MHDF = 0x2F;
constexpr uint8_t REG_NHDF = 0x30;
constexpr uint8_t REG_NCLF = 0x31;
constexpr uint8_t REG_FDLF = 0x32;
constexpr uint8_t REG_NHDT = 0x33;
constexpr uint8_t REG_NCLT = 0x34;
constexpr uint8_t REG_FDLT = 0x35;

constexpr uint8_t SOFTRESET_CMD = 0x63;
constexpr uint8_t DEFAULT_TOUCH_THRESHOLD = 12;
constexpr uint8_t DEFAULT_RELEASE_THRESHOLD = 6;
constexpr uint8_t DEFAULT_ECR = 0x8C;

bool s_i2c_initialized = false;
} // namespace

MPR121_Keypad::MPR121_Keypad(uint8_t address0, uint8_t address1)
    : i2cAddress0(address0),
      i2cAddress1(address1),
      lastTouched0(0),
      lastTouched1(0),
      head(0),
      tail(0),
      count(0) {}

bool MPR121_Keypad::init_i2c() {
    if (s_i2c_initialized) {
        return true;
    }

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = static_cast<gpio_num_t>(TOUCHPAD_SDA);
    conf.scl_io_num = static_cast<gpio_num_t>(TOUCHPAD_SCL);
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_FREQ_HZ;

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return false;
    }

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return false;
    }

    s_i2c_initialized = true;
    return true;
}

bool MPR121_Keypad::write_register(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t payload[2] = {reg, value};
    esp_err_t err = i2c_master_write_to_device(I2C_PORT, address, payload, sizeof(payload), I2C_TIMEOUT_TICKS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "write reg 0x%02X to 0x%02X failed: %s", reg, address, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool MPR121_Keypad::read_registers(uint8_t address, uint8_t reg, uint8_t *data, size_t len) {
    esp_err_t err = i2c_master_write_read_device(I2C_PORT, address, &reg, 1, data, len, I2C_TIMEOUT_TICKS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read reg 0x%02X from 0x%02X failed: %s", reg, address, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool MPR121_Keypad::setup_device(uint8_t address) {
    if (!write_register(address, REG_SOFTRESET, SOFTRESET_CMD)) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1));

    if (!write_register(address, REG_ECR, 0x00)) {
        return false;
    }

    uint8_t config2 = 0;
    if (!read_registers(address, REG_CONFIG2, &config2, 1)) {
        return false;
    }
    if (config2 != 0x24) {
        ESP_LOGE(TAG, "unexpected CONFIG2 at 0x%02X: 0x%02X", address, config2);
        return false;
    }

    for (uint8_t i = 0; i < 12; ++i) {
        if (!write_register(address, REG_TOUCHTH_0 + (2 * i), DEFAULT_TOUCH_THRESHOLD) ||
            !write_register(address, REG_RELEASETH_0 + (2 * i), DEFAULT_RELEASE_THRESHOLD)) {
            return false;
        }
    }

    if (!write_register(address, REG_MHDR, 0x01) ||
        !write_register(address, REG_NHDR, 0x01) ||
        !write_register(address, REG_NCLR, 0x0E) ||
        !write_register(address, REG_FDLR, 0x00) ||
        !write_register(address, REG_MHDF, 0x01) ||
        !write_register(address, REG_NHDF, 0x05) ||
        !write_register(address, REG_NCLF, 0x01) ||
        !write_register(address, REG_FDLF, 0x00) ||
        !write_register(address, REG_NHDT, 0x00) ||
        !write_register(address, REG_NCLT, 0x00) ||
        !write_register(address, REG_FDLT, 0x00) ||
        !write_register(address, REG_DEBOUNCE, 0x00) ||
        !write_register(address, REG_CONFIG1, 0x10) ||
        !write_register(address, REG_CONFIG2, 0x20) ||
        !write_register(address, REG_ECR, DEFAULT_ECR)) {
        return false;
    }

    return true;
}

bool MPR121_Keypad::read_touch_state(uint8_t address, uint16_t *state) {
    if (state == nullptr) {
        return false;
    }

    uint8_t bytes[2] = {0};
    if (!read_registers(address, REG_TOUCH_STATUS_L, bytes, sizeof(bytes))) {
        return false;
    }

    *state = static_cast<uint16_t>((bytes[1] << 8) | bytes[0]) & 0x0FFF;
    return true;
}

bool MPR121_Keypad::begin() {
    if (!init_i2c()) {
        return false;
    }

    if (!setup_device(i2cAddress0) || !setup_device(i2cAddress1)) {
        return false;
    }

    if (!read_touch_state(i2cAddress0, &lastTouched0) || !read_touch_state(i2cAddress1, &lastTouched1)) {
        return false;
    }

    return true;
}

void MPR121_Keypad::scan_device(uint8_t device_index, uint8_t map_offset, uint16_t *last_state) {
    uint16_t current = 0;
    const uint8_t address = (device_index == 0) ? i2cAddress0 : i2cAddress1;

    if (!read_touch_state(address, &current)) {
        return;
    }

    for (uint8_t i = 0; i < 12; ++i) {
        uint16_t mask = static_cast<uint16_t>(1U << i);
        uint8_t mapped_key = TOUCHPAD_MAP[map_offset + i];
        if (mapped_key == 255) {
            continue;
        }

        if ((current & mask) && !(*last_state & mask)) {
            touchKeypadEvent event = {};
            event.bit.KEY = mapped_key;
            event.bit.EVENT = KEY_JUST_PRESSED;
            add_event(event);
        }

        if (!(current & mask) && (*last_state & mask)) {
            touchKeypadEvent event = {};
            event.bit.KEY = mapped_key;
            event.bit.EVENT = KEY_JUST_RELEASED;
            add_event(event);
        }
    }

    *last_state = current;
}

void MPR121_Keypad::tick() {
    scan_device(0, 0, &lastTouched0);
    scan_device(1, 8, &lastTouched1);
}

bool MPR121_Keypad::available() {
    return count > 0;
}

touchKeypadEvent MPR121_Keypad::read() {
    touchKeypadEvent event = {};
    if (count <= 0) {
        return event;
    }

    event = buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    --count;
    return event;
}

void MPR121_Keypad::add_event(touchKeypadEvent e) {
    if (count >= BUFFER_SIZE) {
        tail = (tail + 1) % BUFFER_SIZE;
        --count;
    }

    buffer[head] = e;
    head = (head + 1) % BUFFER_SIZE;
    ++count;
}
