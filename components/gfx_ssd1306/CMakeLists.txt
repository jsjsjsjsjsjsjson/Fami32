idf_component_register(
    SRCS "src/gfx_oled_ssd1306.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd Adafruit-GFX-Library
)

project(gfx_ssd1306)