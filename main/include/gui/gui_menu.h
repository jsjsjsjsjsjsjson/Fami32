#ifndef GUI_MENU_H
#define GUI_MENU_H

#include "gui_common.h"

// Menu management and common UI drawing functions
int menu(const char* name, const char* menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void), uint16_t width, uint16_t height, uint16_t x = 0, uint16_t y = 0, int8_t initPos = 0, int *menuVar = NULL);
int fullScreenMenu(const char* title, const char* options[], uint8_t optionCount, void (*menuActions[])(void), int8_t initPos);
int num_set_menu_int(const char* name, int min, int max, int count, int *num, int x, int y, int width, int height);
void invertRect(int x, int y, int w, int h);
void drawChessboard(int x, int y, int w, int h);
void drawPinstripe(int x, int y, int w, int h);
void drawRectWithNegativeSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void fillRectWithNegativeSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawChessboardWithNegativeSize(int16_t x, int16_t y, int16_t w, int16_t h);

// Main navigation menu (top-level menu to choose Tracker, Channel, etc.)
void menu_navi();

#endif // GUI_MENU_H
