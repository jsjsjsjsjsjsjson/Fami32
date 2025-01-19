#include "micro_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ConfigEntry configEntries[MAX_ENTRIES];
static int entryCount = 0;

int read_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return CONFIG_FILE_ERROR;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[MAX_KEY_LENGTH];
        char value[MAX_STRING_LENGTH];

        if (sscanf(line, "%[^:]:%[^\n]", key, value) != 2) {
            continue;
        }

        ConfigEntry entry;
        entry.key = strdup(key);

        if (value[0] == '"' && value[strlen(value) - 1] == '"') {
            value[strlen(value) - 1] = '\0';
            entry.type = CONFIG_STRING;
            entry.value.stringValue = strdup(value + 1);
        } else {
            char *endptr;
            int intValue = strtol(value, &endptr, 10);
            if (*endptr == '\0') {
                entry.type = CONFIG_INT;
                entry.value.intValue = intValue;
            } else {
                float floatValue = strtof(value, &endptr);
                if (*endptr == '\0') {
                    entry.type = CONFIG_FLOAT;
                    entry.value.floatValue = floatValue;
                } else {
                    entry.type = CONFIG_STRING;
                    entry.value.stringValue = strdup(value);
                }
            }
        }

        if (entryCount < MAX_ENTRIES) {
            configEntries[entryCount++] = entry;
        }
    }

    fclose(file);
    return CONFIG_SUCCESS;
}


int write_config(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file");
        return CONFIG_FILE_ERROR;
    }

    for (int i = 0; i < entryCount; i++) {
        ConfigEntry *entry = &configEntries[i];
        switch (entry->type) {
            case CONFIG_INT:
                fprintf(file, "%s:%d\n", entry->key, entry->value.intValue);
                break;
            case CONFIG_FLOAT:
                fprintf(file, "%s:%f\n", entry->key, entry->value.floatValue);
                break;
            case CONFIG_STRING:
                fprintf(file, "%s:\"%s\"\n", entry->key, entry->value.stringValue);
                break;
        }
    }

    fclose(file);
    return CONFIG_SUCCESS;
}

int get_config_value(const char *key, ValueType type, void *output) {
    for (int i = 0; i < entryCount; i++) {
        if (strcmp(configEntries[i].key, key) == 0) {
            if (configEntries[i].type != type) {
                fprintf(stderr, "Type mismatch for key: %s\n", key);
                return CONFIG_TYPE_MISMATCH;
            }

            switch (type) {
                case CONFIG_INT:
                    *(int *)output = configEntries[i].value.intValue;
                    break;
                case CONFIG_FLOAT:
                    *(float *)output = configEntries[i].value.floatValue;
                    break;
                case CONFIG_STRING:
                    strcpy((char *)output, configEntries[i].value.stringValue);
                    break;
            }
            return CONFIG_SUCCESS;
        }
    }
    fprintf(stderr, "Key not found: %s\n", key);
    return CONFIG_KEY_NOT_FOUND;
}

int set_config_value(const char *key, ValueType type, void *value) {
    for (int i = 0; i < entryCount; i++) {
        if (strcmp(configEntries[i].key, key) == 0) {
            configEntries[i].type = type;
            switch (type) {
                case CONFIG_INT:
                    configEntries[i].value.intValue = *(int *)value;
                    break;
                case CONFIG_FLOAT:
                    configEntries[i].value.floatValue = *(float *)value;
                    break;
                case CONFIG_STRING:
                    free(configEntries[i].value.stringValue);
                    configEntries[i].value.stringValue = strdup((char *)value);
                    break;
            }
            return CONFIG_SUCCESS;
        }
    }

    if (entryCount < MAX_ENTRIES) {
        ConfigEntry entry;
        entry.key = strdup(key);
        entry.type = type;
        switch (type) {
            case CONFIG_INT:
                entry.value.intValue = *(int *)value;
                break;
            case CONFIG_FLOAT:
                entry.value.floatValue = *(float *)value;
                break;
            case CONFIG_STRING:
                entry.value.stringValue = strdup((char *)value);
                break;
        }
        configEntries[entryCount++] = entry;
        return CONFIG_SUCCESS;
    }

    fprintf(stderr, "Maximum number of entries reached\n");
    return CONFIG_FILE_ERROR;
}
