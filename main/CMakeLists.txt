idf_component_register(SRCS "main.cpp" "dpcm.c" "ftm_file.cpp" "tracker.cpp" "micro_config.c" "boot_check.cpp" "note2freq.c" "src_config.c" "wave_table.c" "gen_env.cpp"
                            "gui/gui_common.cpp"
                            "gui/gui_channel.cpp"
                            "gui/gui_file.cpp"
                            "gui/gui_frames.cpp"
                            "gui/gui_info.cpp"
                            "gui/gui_input.cpp"
                            "gui/gui_instrument.cpp"
                            "gui/gui_menu.cpp"
                            "gui/gui_settings.cpp"
                            "gui/gui_tracker.cpp"
                            "gui/gui_visual.cpp"

                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "" "include" "include/gui"
                    REQUIRES Adafruit_SSD1306 Adafruit_Keypad Adafruit_MPR121 arduino-esp32-lite bt nvs_flash USBMIDI espcoredump)

# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -O3)