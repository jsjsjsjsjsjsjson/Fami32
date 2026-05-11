set(APP_SOURCES
    src/app/main.cpp
    src/app/boot_check.cpp
    src/app/boot_router.cpp
    src/app/git_version.cpp
)

set(AUDIO_SOURCES
    src/audio/dpcm.c
    src/audio/gen_env.cpp
    src/audio/note2freq.c
    src/audio/src_config.c
    src/audio/wave_table.c
)

set(CONFIG_SOURCES
    src/config/micro_config.c
)

set(CORE_SOURCES
    src/core/fami32_channel.cpp
    src/core/fami32_instrument.cpp
    src/core/fami32_player.cpp
)

set(GUI_SOURCES
    src/gui/gui_common.cpp
    src/gui/gui_channel.cpp
    src/gui/gui_file.cpp
    src/gui/gui_frames.cpp
    src/gui/gui_info.cpp
    src/gui/gui_input.cpp
    src/gui/note_io.cpp
    src/gui/gui_instrument.cpp
    src/gui/gui_menu.cpp
    src/gui/gui_settings.cpp
    src/gui/gui_tracker.cpp
    src/gui/gui_visual.cpp
    src/gui/gui_dpcm_editor.cpp
)

set(INPUT_SOURCES
    src/input/keypad_io.cpp
    src/input/mpr121_keypad.cpp
    src/input/touch_input.cpp
)

set(STORAGE_SOURCES
    src/storage/ftm_file.cpp
)

set(USB_SOURCES
    src/usb/tinyusb_helper.cpp
    src/usb/uac_mode.cpp
    src/usb/usb_msc_mode.cpp
)

idf_component_register(
    SRCS
        ${APP_SOURCES}
        ${AUDIO_SOURCES}
        ${CONFIG_SOURCES}
        ${CORE_SOURCES}
        ${GUI_SOURCES}
        ${INPUT_SOURCES}
        ${STORAGE_SOURCES}
        ${USB_SOURCES}
    PRIV_REQUIRES spi_flash
    INCLUDE_DIRS
        include
        include/app
        include/assets
        include/audio
        include/config
        include/fami32core
        include/gui
        include/hardware
        include/input
        include/storage
        include/usb
        include/utils
        ${CMAKE_BINARY_DIR}
    REQUIRES gfx_ssd1306 bt nvs_flash USBMIDI espcoredump esp_tinyusb
)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/app/git_version.h.in
    ${CMAKE_BINARY_DIR}/git_version.h
)
