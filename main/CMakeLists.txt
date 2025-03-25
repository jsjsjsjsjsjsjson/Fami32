idf_component_register(SRCS "main.cpp" "dpcm.c" "ftm_file.cpp" "tracker.cpp" "ls.c" "micro_config.c" "boot_check.cpp" "note2freq.c" "src_config.c" "wave_table.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "" "include"
                    REQUIRES Adafruit_SSD1306 Adafruit_Keypad Adafruit_MPR121 arduino-esp32-lite bt nvs_flash USBMIDI espcoredump)

# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -O3)