#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#define CONFIG_SUCCESS 0
#define CONFIG_TYPE_MISMATCH -1
#define CONFIG_FILE_ERROR -2
#define CONFIG_KEY_NOT_FOUND -3

#define MAX_ENTRIES 100
#define MAX_KEY_LENGTH 50
#define MAX_STRING_LENGTH 100

typedef enum {
    CONFIG_INT,
    CONFIG_FLOAT,
    CONFIG_STRING
} ValueType;

typedef struct {
    char *key;
    ValueType type;
    union {
        int intValue;
        float floatValue;
        char *stringValue;
    } value;
} ConfigEntry;

int read_config(const char *filename);
int write_config(const char *filename);
int get_config_value(const char *key, ValueType type, void *output);
int set_config_value(const char *key, ValueType type, void *value);

#endif
