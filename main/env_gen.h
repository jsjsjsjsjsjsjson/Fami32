#ifndef ENV_GEN_H
#define ENV_GEN_H

#include <math.h>

#define MATH_ENV_NUM 5

const char *math_env_name[MATH_ENV_NUM] = {
    "SQUARE", "SQUART", "EXP", "SIN", "LINEAR"
};

float (*mathEnv[MATH_ENV_NUM][2])(int x, float a, int l) = {
    { 
        [](int x, float a, int l) { return sqrtf(a * x * (1.0f / l)); }, 
        [](int x, float a, int l) { return sqrtf(((-a * x) + 1.0f) * (1.0f / l)); }
    },
    {
        [](int x, float a, int l) { return exp2f(a * x * (1.0f / l)); },
        [](int x, float a, int l) { return 1.0f - exp2f(a * x * (1.0f / l)); }
    },
    {
        [](int x, float a, int l) { return expf(-a * x * (1.0f / l)); },
        [](int x, float a, int l) { return expf((a * x * (1.0f / l)) - a); }
    },
    {
        [](int x, float a, int l) { return sinf(a * x * (M_PI / (2.0f * l))); },
        [](int x, float a, int l) { return sinf(-a * x * (M_PI / (2.0f * l))) + 1.0f; }
    },
    {
        [](int x, float a, int l) { return a * x * (1.0f / l); },
        [](int x, float a, int l) { return (-a * x * (1.0f / l)) + 1.0f; }
    }
};

#define CHORD_ENV_NUM 39

const char *chordNames[CHORD_ENV_NUM] = {
    "C Major",            // 大三和弦 (C major)
    "C Minor",            // 小三和弦 (C minor)
    "C Major 7",          // 大七和弦 (Cmaj7)
    "C Minor 7",          // 小七和弦 (Cm7)
    "C7b5",               // 大七减五和弦 (C7b5)
    "C9",                 // 大九和弦 (C9)
    "Cmaj#5",             // 大增五和弦 (Cmaj#5)
    "Cm9",                // 小九和弦 (Cm9)

    "Cm7",                // 小七和弦 (Cm7)
    "Caug",               // 增三和弦 (Caug)
    "Cø7",                // 半减七和弦 (Cø7)
    "Cø9",                // 半减九和弦 (Cø9)
    "C Lydian",           // Lydian和弦 (C Lydian)
    "C Lydian #5",        // Lydian Augmented (C Lydian #5)
    "Cm7",                // 小七和弦 (Cm7)
    "Cm+9",               // 小加九和弦 (Cm+9)
    "C+7",                // 增七和弦 (C+7)

    "Cm+3",               // 小增三和弦 (Cm+3)
    "Cmaj-min",           // 大小三和弦 (Cmaj-min)
    "Cm+9",               // 小加九和弦 (Cm+9)
    "Cm5",                // 小五和弦 (Cm5)
    "C9+5",               // 大九加五和弦 (C9+5)
    "C9+6",               // 大九加六和弦 (C9+6)
    "C+4",                // 增四和弦 (C+4)
    "C+9",                // 增九和弦 (C+9)
    "Cdim6",              // C减六和弦 (Cdim6)
    "Cm6",                // 小七加六和弦 (Cm6)
    "Cmaj7+5",            // 大七加五和弦 (Cmaj7+5)

    "Caug5",              // 增五和弦 (Caug5)
    "Cm+",                // 小增和弦 (Cm+)
    "C+5",                // 完美五和弦 (C+5)
    "Cm7+",               // 小增七和弦 (Cm7+)
    "C+5-7",              // 大增五减七 (C+5-7)
    "Cm7+2",              // 小七和弦加二度 (Cm7+2)
    "Cmaj9-5",            // 大三小九和弦 (Cmaj9-5)
    "CmMaj7",             // 小大七和弦 (CmMaj7)
    "C+3add4",            // 增三和弦加四 (C+3add4)
    "C+5+8",              // C增五和弦加八 (C+5+8)
    "C Half step chord"   // 半音和弦 (C Half step chord)
};

int chordLengths[CHORD_ENV_NUM] = {
    3,  // C Major
    3,  // C Minor
    4,  // C Major 7
    4,  // C Minor 7
    4,  // C7b5
    5,  // C9
    4,  // Cmaj#5
    5,  // Cm9
    4,  // Cm7
    3,  // Caug
    4,  // Cø7
    5,  // Cø9
    5,  // C Lydian
    5,  // C Lydian #5
    4,  // Cm7
    4,  // Cm+9
    5,  // C+7
    4,  // Cm+3
    4,  // Cmaj-min
    5,  // Cm+9
    4,  // Cm5
    5,  // C9+5
    5,  // C9+6
    5,  // C+4
    4,  // C+9
    4,  // Cdim6
    4,  // Cm6
    5,  // Cmaj7+5
    4,  // Caug5
    4,  // Cm+
    4,  // C+5
    4,  // Cm7+
    4,  // C+5-7
    4,  // Cm7+2
    4,  // Cmaj9-5
    5,  // CmMaj7
    4,  // C+3add4
    4,  // C+5+8
    5   // C Half step chord
};

char chords[CHORD_ENV_NUM][5] = {
    {0, 4, 7},          // 大三和弦 (C major)
    {0, 3, 7},          // 小三和弦 (C minor)
    {0, 4, 7, 11},      // 大七和弦 (Cmaj7)
    {0, 3, 7, 10},      // 小七和弦 (Cm7)
    {0, 4, 7, 10},      // 大七减五和弦 (C7b5)
    {0, 4, 7, 10, 14},  // 大九和弦 (C9)
    {0, 4, 8, 11},      // 大增五和弦 (Cmaj#5)
    {0, 3, 7, 10, 13},  // 小九和弦 (Cm9)

    {0, 3, 7, 10, 14},  // 小七和弦 (Cm7)
    {0, 5, 9},          // 增三和弦 (Caug)
    {0, 3, 6, 10},      // 半减七和弦 (Cø7)
    {0, 3, 6, 10, 14},  // 半减九和弦 (Cø9)
    {0, 2, 5, 9, 12},   // Lydian和弦 (C Lydian)
    {0, 2, 4, 7, 11},   // Lydian Augmented (C Lydian #5)
    {0, 3, 7, 10},      // 小七和弦 (Cm7)
    {0, 3, 7, 9},       // 小加九和弦 (Cm+9)
    {0, 5, 7, 10, 14},  // 增七和弦 (C+7)

    {0, 3, 6, 9},       // 小增三和弦 (Cm+3)
    {0, 2, 7, 10},      // 大小三和弦 (Cmaj-min)
    {0, 3, 6, 9, 14},   // 小加九和弦 (Cm+9)
    {0, 3, 5, 7, 10},   // 小五和弦 (Cm5)
    {0, 4, 7, 11, 14},  // 大九加五和弦 (C9+5)
    {0, 4, 7, 11, 15},  // 大九加六和弦 (C9+6)
    {0, 2, 5, 8, 12},   // 增四和弦 (C+4)
    {0, 2, 7, 10},      // 增九和弦 (C+9)
    {0, 2, 6, 9},       // C减六和弦 (Cdim6)
    {0, 3, 6, 7},       // 小三和弦加六和弦 (Cm6)
    {0, 3, 7, 11, 14},  // 大七和弦加五和弦 (Cmaj7+5)

    {0, 5, 8, 12},      // 增五和弦 (Caug5)
    {0, 3, 6, 9},       // 小增和弦 (Cm+)
    {0, 2, 5, 7, 10},   // 完美五和弦 (C+5)
    {0, 2, 5, 7, 11},   // 小增七和弦 (Cm7+)
    {0, 4, 8, 10},      // 大增五减七 (C+5-7)
    {0, 3, 6, 8},       // 小七和弦加二度 (Cm7+2)
    {0, 4, 6, 9},       // 大三小九和弦 (Cmaj9-5)
    {0, 3, 7, 10, 13},  // 小大七和弦 (CmMaj7)
    {0, 2, 5, 7, 10},   // 增三和弦加四 (C+3add4)
    {0, 4, 8, 12, 16},  // C增五和弦加八 (C+5+8)
    {0, 1, 3, 5, 7},    // 半音和弦 (C Half step chord)
};

#endif