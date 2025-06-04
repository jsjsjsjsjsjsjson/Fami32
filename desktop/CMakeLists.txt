cmake_minimum_required(VERSION 3.10)
project(Fami32Desktop)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS}
                    ../main/include
                    ../main/include/fami32core)

add_definitions(-DDESKTOP_BUILD)

set(SRC
    ../main/fami32core/fami32_channel.cpp
    ../main/fami32core/fami32_instrument.cpp
    ../main/fami32core/fami32_player.cpp
    ../main/ftm_file.cpp
    ../main/dpcm.c
    ../main/note2freq.c
    ../main/wave_table.c
    ../main/micro_config.c
    ../main/src_config.c
    ../main/gen_env.cpp
    main.cpp
)

add_executable(fami32_desktop ${SRC})
target_link_libraries(fami32_desktop SDL2::SDL2 SDL2::SDL2main)
