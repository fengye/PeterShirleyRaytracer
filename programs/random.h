#ifndef _H_RANDOM_
#define _H_RANDOM_

#include <stdlib.h>

#if defined(_SPU_) || defined(_PPU_)

inline double random_double()
{
    // [0, 1)
    return (double)rand() / ((double)RAND_MAX + 1);
}

inline double random_double_range(double min, double max)
{
    // [min, max)
    return min + (max - min) * random_double();
}

inline float random_float()
{
    // [0, 1)
    return (float)rand() / ((float)RAND_MAX + 1);
}

inline float random_float_range(float min, float max)
{
    // [min, max)
    return min + (max - min) * random_float();
}

#endif

#endif //_H_RANDOM_