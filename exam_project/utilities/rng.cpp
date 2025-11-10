#include <bits/stdc++.h>
#include <ctime>
#include <random>
using namespace std;

struct PRNG
{
    mt19937 rng;
};

PRNG* init_rng(){
    PRNG ret;
    ret.rng = mt19937(time({}));
    return &ret;
}

uint32_t random_up_to(uint32_t range, PRNG* engine){
    uint32_t x = engine->rng();
    uint64_t m = uint64_t(x) * uint64_t(range);
    return m >> 32;
}

/**
 * Generates random number in range [min,max)
 * Assumes that min < max
 */
uint32_t random_in_range(uint32_t min, uint32_t max, PRNG* engine){
    uint32_t ran = max - min;
    return random_up_to(ran ,engine) + min;
}