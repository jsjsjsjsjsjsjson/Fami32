cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "Adafruit_MPR121.cpp" "MPR121_Keypad.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES Adafruit_BusIO Adafruit_Keypad arduino-esp32-lite)

project(Adafruit_MPR121)
