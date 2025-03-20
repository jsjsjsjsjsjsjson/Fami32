# Adafruit Keypad Library
# MIT License

cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "Adafruit_Keypad.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES arduino-esp32-lite)

project(Adafruit_Keypad)
