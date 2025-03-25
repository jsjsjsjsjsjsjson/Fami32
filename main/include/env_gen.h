#ifndef ENV_GEN_H
#define ENV_GEN_H

#include <math.h>

#define MATH_ENV_NUM 5
#define CHORD_ENV_NUM 39

extern const char *math_env_name[MATH_ENV_NUM];
extern float (*mathEnv[MATH_ENV_NUM])(int x, float a, int l);

extern const char *chordNames[CHORD_ENV_NUM];
extern int chordLengths[CHORD_ENV_NUM];
extern char chords[CHORD_ENV_NUM][5];

#endif