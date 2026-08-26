#include "erl_math.h"

RNG *stateRng = nullptr;

void init_rng(RNG *rng) {
    stateRng = rng;
}

u64 rng_next() {
    u64 x = stateRng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    stateRng->state = x;
    return x * 2685821657736338717ULL;
}

f32 rng_next_f32() {
    return (rng_next() >> 40) * (1.0f / (1ULL << 24));
}

i32 rng_range(i32 min, i32 max) {
    if (max < min) {
        i32 tmp = min;
        min = max;
        max = tmp;
    }

    i32 range = max - min + 1;
    if (range == 0) return min;
    u64 r = rng_next();                
    return (i32)(r % range) + min;        
}
