#ifndef FREE_HEAP_H
#define FREE_HEAP_H

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void free_command(int argc, const char* argv[]) {
    // 默认单位为KB
    char unit = 'k';
    float factor = 1024.0f;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) {
            unit = 'B';
            factor = 1.0f;
        } else if (strcmp(argv[i], "-k") == 0) {
            unit = 'K';
            factor = 1024.0f;
        } else if (strcmp(argv[i], "-m") == 0) {
            unit = 'M';
            factor = 1024.0f * 1024.0f;
        } else if (strcmp(argv[i], "-h") == 0) {
            // 自动选择单位
            size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
            if (total_heap >= (1024.0f * 1024.0f)) {
                unit = 'M';
                factor = 1024.0f * 1024.0f;
            } else if (total_heap >= 1024.0f) {
                unit = 'K';
                factor = 1024.0f;
            } else {
                unit = 'B';
                factor = 1.0f;
            }
        } else {
            printf("free: option requires an argument -- '%c'\n", argv[i][1]);
            printf("Usage: free [-b|-k|-m|-h]\n");
            return;
        }
    }

    // 获取内存信息
    size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    size_t free_heap = esp_get_free_heap_size();
    size_t min_free_heap = esp_get_minimum_free_heap_size();

    // 计算内存值
    float total = (float)total_heap / factor;
    float free_mem = (float)free_heap / factor;
    float used_mem = total - free_mem;
    float available_mem = (float)min_free_heap / factor;

    // 输出内存信息
    printf("              total        free      used      available\n");
    printf("Memory:    %.2f%c        %.2f%c      %.2f%c        %.2f%c\n",
           total, unit,
           free_mem, unit,
           used_mem, unit,
           available_mem, unit);
}

#endif