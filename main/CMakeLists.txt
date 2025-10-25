idf_component_register(SRCS "main.cpp" "dpcm.c" "ftm_file.cpp" "micro_config.c" "boot_check.cpp" "note2freq.c" "src_config.c" "wave_table.c" "gen_env.cpp"
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
                            "gui/gui_dpcm_editor.cpp"

                            "fami32core/fami32_channel.cpp"
                            "fami32core/fami32_instrument.cpp"
                            "fami32core/fami32_player.cpp"

                            "git_version.cpp"

                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "include" "include/gui" "include/fami32core"
                    REQUIRES Adafruit_SSD1306 Adafruit_Keypad Adafruit_MPR121 arduino-esp32-lite bt nvs_flash USBMIDI espcoredump)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH_FULL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --tags --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --quiet HEAD --
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_HAS_CHANGES
    )
    
    if(GIT_HAS_CHANGES EQUAL 0)
        set(GIT_IS_DIRTY "false")
    else()
        set(GIT_IS_DIRTY "true")
    endif()
    
else()
    set(GIT_COMMIT_HASH_FULL "unknown")
    set(GIT_COMMIT_HASH_SHORT "unknown")
    set(GIT_BRANCH "unknown")
    set(GIT_DESCRIBE "unknown")
    set(GIT_IS_DIRTY "unknown")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/main/include/git_version.h.in
    ${CMAKE_BINARY_DIR}/git_version.h
)