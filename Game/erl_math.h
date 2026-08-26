#ifndef MATH_H
#define MATH_H
#include "data_types.h"

struct RNG {
    u64 state;
};

void init_rng(RNG *rng);
f32 rng_next_f32();
i32 rng_range(i32 min, i32 max);

#endif
