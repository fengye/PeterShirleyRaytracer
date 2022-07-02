#ifndef _H_RANDOM_
#define _H_RANDOM_

inline double random_double()
{
    // [0, 1)
    return (double)rand() / ((double)RAND_MAX + 1);
}

inline double random_double(double min, double max)
{
    // [min, max)
    return min + (max - min) * random_double();
}



#endif //_H_RANDOM_