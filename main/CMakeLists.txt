idf_component_register(SRCS "main.cpp" "dpcm.c" "ftm_file.cpp" "ls.c" "micro_config.c"
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "" "include"
                    REQUIRES Adafruit_SSD1306 Adafruit_Keypad Adafruit_MPR121 arduino-esp32-lite bt nvs_flash USBMIDI)

# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -O3)
littlefs_create_partition_image(flash test_file)