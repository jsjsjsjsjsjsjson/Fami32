#include "micro_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define LINE_BUFFER_SIZE 512

static ConfigEntry configEntries[MAX_ENTRIES];
static int entryCount = 0;

/* ---------- Internal helpers ---------- */

static int is_valid_type(ValueType type) {
    return (type == CONFIG_INT ||
            type == CONFIG_FLOAT ||
            type == CONFIG_STRING);
}

static void free_entry(ConfigEntry *entry) {
    if (!entry) {
        return;
    }

    free(entry->key);
    entry->key = NULL;

    if (entry->type == CONFIG_STRING) {
        free(entry->value.stringValue);
        entry->value.stringValue = NULL;
    }
}

static void clear_all_entries(void) {
    for (int i = 0; i < entryCount; ++i) {
        free_entry(&configEntries[i]);
    }
    memset(configEntries, 0, sizeof(configEntries));
    entryCount = 0;
}

static char *trim_left(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        ++s;
    }
    return s;
}

static void trim_right_inplace(char *s) {
    size_t len;

    if (!s) {
        return;
    }

    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        --len;
    }
}

static char *trim(char *s) {
    s = trim_left(s);
    trim_right_inplace(s);
    return s;
}

static int validate_key(const char *key) {
    size_t len;

    if (!key) {
        return 0;
    }

    len = strnlen(key, MAX_KEY_LENGTH + 1);
    if (len == 0 || len > MAX_KEY_LENGTH) {
        return 0;
    }

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)key[i];
        if (!(isalnum(c) || c == '_' || c == '-' || c == '.')) {
            return 0;
        }
    }

    return 1;
}

static char *dup_bounded(const char *src, size_t maxLen) {
    size_t len;
    char *dst;

    if (!src) {
        return NULL;
    }

    len = strnlen(src, maxLen + 1);
    if (len > maxLen) {
        return NULL;
    }

    dst = (char *)malloc(len + 1);
    if (!dst) {
        return NULL;
    }

    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

static int find_entry_index(const char *key) {
    if (!key) {
        return -1;
    }

    for (int i = 0; i < entryCount; ++i) {
        if (configEntries[i].key && strcmp(configEntries[i].key, key) == 0) {
            return i;
        }
    }

    return -1;
}

static int unescape_string(const char *src, char *dst, size_t dstSize) {
    size_t di = 0;
    size_t si = 0;

    if (!src || !dst || dstSize == 0) {
        return -1;
    }

    while (src[si] != '\0') {
        char c = src[si++];

        if (c == '\\') {
            char esc = src[si++];
            switch (esc) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '\0': return -1;
                default: return -1;
            }
        }

        if (di + 1 >= dstSize) {
            return -1;
        }

        dst[di++] = c;
    }

    dst[di] = '\0';
    return 0;
}

static int parse_quoted_string(const char *src, char *out, size_t outSize) {
    size_t len;
    char inner[MAX_STRING_LENGTH * 2];

    if (!src || !out || outSize == 0) {
        return -1;
    }

    len = strlen(src);
    if (len < 2) {
        return -1;
    }
    if (src[0] != '"' || src[len - 1] != '"') {
        return -1;
    }

    if (len - 2 >= sizeof(inner)) {
        return -1;
    }

    memcpy(inner, src + 1, len - 2);
    inner[len - 2] = '\0';

    return unescape_string(inner, out, outSize);
}

static void escape_string_to_file(FILE *file, const char *s) {
    while (*s) {
        switch (*s) {
            case '\n': fputs("\\n", file); break;
            case '\r': fputs("\\r", file); break;
            case '\t': fputs("\\t", file); break;
            case '\\': fputs("\\\\", file); break;
            case '"':  fputs("\\\"", file); break;
            default: fputc(*s, file); break;
        }
        ++s;
    }
}

static int assign_entry_value(ConfigEntry *entry, ValueType type, const void *value) {
    if (!entry || !value || !is_valid_type(type)) {
        return CONFIG_FILE_ERROR;
    }

    if (entry->type == CONFIG_STRING) {
        free(entry->value.stringValue);
        entry->value.stringValue = NULL;
    }

    entry->type = type;

    switch (type) {
        case CONFIG_INT:
            entry->value.intValue = *(const int *)value;
            return CONFIG_SUCCESS;

        case CONFIG_FLOAT:
            entry->value.floatValue = *(const float *)value;
            return CONFIG_SUCCESS;

        case CONFIG_STRING: {
            const char *src = (const char *)value;
            char *copy = dup_bounded(src, MAX_STRING_LENGTH - 1);
            if (!copy) {
                return CONFIG_FILE_ERROR;
            }
            entry->value.stringValue = copy;
            return CONFIG_SUCCESS;
        }

        default:
            return CONFIG_FILE_ERROR;
    }
}

static int parse_line_and_store(const char *line) {
    char buf[LINE_BUFFER_SIZE];
    char *work;
    char *eq;
    char *key;
    char *rhs;
    char *colon;
    char *typeStr;
    char *valueStr;

    if (!line) {
        return CONFIG_FILE_ERROR;
    }

    if (strnlen(line, sizeof(buf)) >= sizeof(buf)) {
        return CONFIG_FILE_ERROR;
    }

    strcpy(buf, line);
    work = trim(buf);

    if (*work == '\0' || *work == '#' || *work == ';') {
        return CONFIG_SUCCESS;
    }

    eq = strchr(work, '=');
    if (!eq) {
        return CONFIG_FILE_ERROR;
    }

    *eq = '\0';
    key = trim(work);
    rhs = trim(eq + 1);

    if (!validate_key(key)) {
        return CONFIG_FILE_ERROR;
    }

    colon = strchr(rhs, ':');
    if (!colon) {
        return CONFIG_FILE_ERROR;
    }

    *colon = '\0';
    typeStr = trim(rhs);
    valueStr = trim(colon + 1);

    if (strcmp(typeStr, "i") == 0) {
        char *endptr;
        long v;

        errno = 0;
        v = strtol(valueStr, &endptr, 10);
        if (errno != 0) {
            return CONFIG_FILE_ERROR;
        }
        endptr = trim(endptr);
        if (*endptr != '\0') {
            return CONFIG_FILE_ERROR;
        }

        {
            int iv = (int)v;
            return set_config_value(key, CONFIG_INT, &iv);
        }
    }

    if (strcmp(typeStr, "f") == 0) {
        char *endptr;
        float v;

        errno = 0;
        v = strtof(valueStr, &endptr);
        if (errno != 0) {
            return CONFIG_FILE_ERROR;
        }
        endptr = trim(endptr);
        if (*endptr != '\0') {
            return CONFIG_FILE_ERROR;
        }

        return set_config_value(key, CONFIG_FLOAT, &v);
    }

    if (strcmp(typeStr, "s") == 0) {
        char parsed[MAX_STRING_LENGTH];
        if (parse_quoted_string(valueStr, parsed, sizeof(parsed)) != 0) {
            return CONFIG_FILE_ERROR;
        }
        return set_config_value(key, CONFIG_STRING, parsed);
    }

    return CONFIG_FILE_ERROR;
}

static int build_temp_filename(const char *filename, char *temp, size_t tempSize) {
    int ret;

    if (!filename || !temp || tempSize == 0) {
        return -1;
    }

    ret = snprintf(temp, tempSize, "%s.tmp", filename);
    if (ret < 0 || (size_t)ret >= tempSize) {
        return -1;
    }

    return 0;
}

/* ---------- Public API ---------- */

int read_config(const char *filename) {
    FILE *file;
    char line[LINE_BUFFER_SIZE];

    if (!filename) {
        return CONFIG_FILE_ERROR;
    }

    file = fopen(filename, "r");
    if (!file) {
        return CONFIG_FILE_ERROR;
    }

    clear_all_entries();

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);

        if (len == sizeof(line) - 1 && line[len - 1] != '\n') {
            fclose(file);
            clear_all_entries();
            return CONFIG_FILE_ERROR;
        }

        trim_right_inplace(line);

        if (parse_line_and_store(line) != CONFIG_SUCCESS) {
            fclose(file);
            clear_all_entries();
            return CONFIG_FILE_ERROR;
        }
    }

    fclose(file);
    return CONFIG_SUCCESS;
}

int write_config(const char *filename) {
    FILE *file;
    char tempFilename[512];

    if (!filename) {
        return CONFIG_FILE_ERROR;
    }

    if (build_temp_filename(filename, tempFilename, sizeof(tempFilename)) != 0) {
        return CONFIG_FILE_ERROR;
    }

    file = fopen(tempFilename, "w");
    if (!file) {
        return CONFIG_FILE_ERROR;
    }

    fputs("# micro_config generated file\n", file);
    fputs("# format: key = <type>:<value>\n", file);
    fputs("# types : i=int, f=float, s=string\n\n", file);

    for (int i = 0; i < entryCount; ++i) {
        ConfigEntry *entry = &configEntries[i];

        if (!entry->key || !is_valid_type(entry->type)) {
            fclose(file);
            remove(tempFilename);
            return CONFIG_FILE_ERROR;
        }

        fprintf(file, "%s = ", entry->key);

        switch (entry->type) {
            case CONFIG_INT:
                fprintf(file, "i:%d\n", entry->value.intValue);
                break;

            case CONFIG_FLOAT:
                fprintf(file, "f:%.9g\n", entry->value.floatValue);
                break;

            case CONFIG_STRING:
                if (!entry->value.stringValue) {
                    fclose(file);
                    remove(tempFilename);
                    return CONFIG_FILE_ERROR;
                }
                fputs("s:\"", file);
                escape_string_to_file(file, entry->value.stringValue);
                fputs("\"\n", file);
                break;

            default:
                fclose(file);
                remove(tempFilename);
                return CONFIG_FILE_ERROR;
        }
    }

    if (fflush(file) != 0) {
        fclose(file);
        remove(tempFilename);
        return CONFIG_FILE_ERROR;
    }

    fclose(file);

    if (rename(tempFilename, filename) != 0) {
        remove(tempFilename);
        return CONFIG_FILE_ERROR;
    }

    return CONFIG_SUCCESS;
}

int get_config_value(const char *key, ValueType type, void *output) {
    int index;

    if (!key || !output || !is_valid_type(type)) {
        return CONFIG_FILE_ERROR;
    }

    index = find_entry_index(key);
    if (index < 0) {
        return CONFIG_KEY_NOT_FOUND;
    }

    if (configEntries[index].type != type) {
        return CONFIG_TYPE_MISMATCH;
    }

    switch (type) {
        case CONFIG_INT:
            *(int *)output = configEntries[index].value.intValue;
            return CONFIG_SUCCESS;

        case CONFIG_FLOAT:
            *(float *)output = configEntries[index].value.floatValue;
            return CONFIG_SUCCESS;

        case CONFIG_STRING: {
            size_t len;
            char *dst = (char *)output;
            const char *src = configEntries[index].value.stringValue;

            if (!src) {
                return CONFIG_FILE_ERROR;
            }

            len = strnlen(src, MAX_STRING_LENGTH - 1);
            memcpy(dst, src, len);
            dst[len] = '\0';
            return CONFIG_SUCCESS;
        }

        default:
            return CONFIG_FILE_ERROR;
    }
}

int set_config_value(const char *key, ValueType type, void *value) {
    int index;

    if (!key || !value || !is_valid_type(type)) {
        return CONFIG_FILE_ERROR;
    }

    if (!validate_key(key)) {
        return CONFIG_FILE_ERROR;
    }

    if (type == CONFIG_STRING) {
        if (strnlen((const char *)value, MAX_STRING_LENGTH) >= MAX_STRING_LENGTH) {
            return CONFIG_FILE_ERROR;
        }
    }

    index = find_entry_index(key);
    if (index >= 0) {
        return assign_entry_value(&configEntries[index], type, value);
    }

    if (entryCount >= MAX_ENTRIES) {
        return CONFIG_FILE_ERROR;
    }

    memset(&configEntries[entryCount], 0, sizeof(configEntries[entryCount]));

    configEntries[entryCount].key = dup_bounded(key, MAX_KEY_LENGTH);
    if (!configEntries[entryCount].key) {
        return CONFIG_FILE_ERROR;
    }

    configEntries[entryCount].type = type;

    if (assign_entry_value(&configEntries[entryCount], type, value) != CONFIG_SUCCESS) {
        free_entry(&configEntries[entryCount]);
        return CONFIG_FILE_ERROR;
    }

    ++entryCount;
    return CONFIG_SUCCESS;
}