# Fami32 - FamiTracker on ESP32

## Overview
Fami32 is a standalone chiptune editor and player for the ESP32 microcontroller. The project aims to mimic core features of **FamiTracker** so that FTM modules can be edited and played directly on hardware. It targets the classic 2A03 sound chip and provides basic support for DPCM sample playback.

The application exposes its file storage as a USB mass storage device and also offers USB MIDI output. Configuration parameters are stored on the flash file system, allowing settings to persist between reboots.

## Directory Layout
- **`main/`** – Application source code
  - `fami32core/` – Audio engine implementation
  - `gui/` – User interface and menu system
  - `include/` – Public headers (pin assignments, fonts, etc.)
- **`components/`** – Third-party libraries used by the project
  - `Adafruit_GFX_Library`, `Adafruit_SSD1306`, `Adafruit_Keypad`, `Adafruit_MPR121`, `Adafruit_BusIO`
  - `USBMIDI` – simple USB MIDI helper
  - `arduino-esp32-lite` – minimal Arduino core for ESP32
- **`CMakeLists.txt`** – Top-level build definition
- **`sdkconfig`** – ESP-IDF configuration
- **`partitions.csv`** – Flash partition scheme (includes `/flash` FAT partition)

## Building
1. Install **ESP-IDF 5.4+** and export the environment (`. $IDF_PATH/export.sh`).
2. Configure the project if necessary: `idf.py menuconfig`.
3. Build the firmware:
   ```bash
   idf.py set-target esp32s3
   idf.py build
   ```
4. Flash to your device:
   ```bash
   idf.py -p <target COM> flash monitor
   ```

## Usage
Upon reset the device mounts the internal FAT partition as `/flash` and exposes it over USB. Copy FTM modules to this storage to load them from the file menu. A configuration file is stored at `/flash/FM32CONF.CNF` and contains settings such as sample rate, engine speed and filter cut-offs.

Hardware controls are defined in `main/include/fami32_pin.h` and include a keypad matrix and two MPR121 touch controllers. The OLED display uses the SSD1309/SSD1306 driver via SPI. Audio is produced through an external PCM5102A DAC using I2S.

The main tracker interface provides pattern editing similar to FamiTracker. Keys allow navigation, editing and playback control while the touch pads can be used for entering notes. The option menu offers access to settings, file operations and instrument parameters.

## Configuration Parameters
Settings are edited via the on device menu and saved to `FM32CONF.CNF`. Examples include:
- `SAMPLE_RATE` – audio sample rate (default 96000)
- `ENGINE_SPEED` – ticks per second
- `LPF_CUTOFF` / `HPF_CUTOFF` – filter settings
- `BASE_FREQ_HZ` – tuning reference
- `OVER_SAMPLE` – oversampling factor
- `VOLUME` – master volume

Manual edits can also be made by mounting the mass storage device and editing the file directly.

## Contributing
Pull requests and bug reports are welcome. Third-party libraries retain their original licenses; see each component directory for details.

## License
This project is distributed under the terms of the MIT license. See individual component directories for their respective licenses.

## Support me
​I created an Afdian!
I hope that the little things I do can bring in some income.
So if you can, buy me a coffee ☕ !
I would be very grateful ✨ 
https://www.afdian.com/a/libchara
