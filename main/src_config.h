#ifndef SRC_CONFIG_H
#define SRC_CONFIG_H

#define FAMI32_VERSION 1
#define FAMI32_SUBVERSION 6

int SAMP_RATE = 96000;
int ENG_SPEED = 60;

int LPF_CUTOFF = 18000;
int HPF_CUTOFF = 32;

int OVER_SAMPLE = 4;

const char *config_path = "/flash/FM32CONF.CNF";

#endif