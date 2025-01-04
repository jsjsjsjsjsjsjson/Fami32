#ifndef LS_H
#define LS_H

#include <stddef.h>


void print_file_details(const char *path, const char *filename);

void ls_command(const char *path, int show_details);

void ls_entry(int argc, const char *argv[]);

#endif  // LS_H
