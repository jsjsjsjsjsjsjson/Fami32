#ifndef GUI_FILE_H
#define GUI_FILE_H

#include "gui_common.h"

// File menu and file selection functions
void menu_file();
void open_file_page();
void export_vgm_page();
typedef bool (*file_select_accept_fn)(const char *path, void *user);
const char* file_select(const char *basePath, file_select_accept_fn accept = NULL, void *user = NULL);

#endif // GUI_FILE_H
