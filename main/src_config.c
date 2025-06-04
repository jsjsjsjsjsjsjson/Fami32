#include "src_config.h"

int SAMP_RATE = 96000;
int ENG_SPEED = 60;
int LPF_CUTOFF = 18000;
int HPF_CUTOFF = 32;
int OVER_SAMPLE = 4;
#ifdef DESKTOP_BUILD
const char *config_path = "FM32CONF.CNF";
#else
const char *config_path = "/flash/FM32CONF.CNF";
#endif
