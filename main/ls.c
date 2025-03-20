#include "ls.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

void print_file_details(const char *path, const char *filename) {
    struct stat file_stat;
    char full_path[512];

    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    if (stat(full_path, &file_stat) == -1) {
        perror("stat");
        return;
    }

    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

    printf(" %ld", file_stat.st_size);

    char time_buffer[100];
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M", tm_info);
    printf(" %s", time_buffer);

    printf(" %s\n", filename);
}

void ls_command(const char *path, int show_details) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (show_details) {
                print_file_details(path, entry->d_name);
            } else {
                printf("%s  ", entry->d_name);
            }
        }
    }

    if (!show_details) {
        printf("\n");
    }

    closedir(dir);
}

void ls_entry(int argc, const char *argv[]) {
    int show_details = 0;
    const char *dir_path = "/flash";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            show_details = 1;
        } else {
            dir_path = argv[i];
        }
    }

    ls_command(dir_path, show_details);
}
