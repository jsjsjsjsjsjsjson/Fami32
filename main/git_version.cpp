#include "../build/git_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* get_version_string(void) {
    static char version_str[256] = {0};
    
    if (version_str[0] == '\0') {
        if (GIT_IS_DIRTY) {
            snprintf(version_str, sizeof(version_str), 
                    "%s^", GIT_COMMIT_HASH_SHORT);
        } else {
            snprintf(version_str, sizeof(version_str), 
                    "%s", GIT_COMMIT_HASH_SHORT);
        }
    }
    
    return version_str;
}