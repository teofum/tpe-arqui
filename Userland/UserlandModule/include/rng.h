#ifndef __RNG_H__
#define __RNG_H__
#include <stdint.h>

// https://github.com/imneme/pcg-c-basic/blob/master/pcg_basic.c
typedef struct {
  uint64_t state, inc;
} pcg32_random_t;

void pcg32_srand(pcg32_random_t *rng, uint64_t initstate, uint64_t initseq);

uint32_t pcg32_rand(pcg32_random_t *rng);

#endif