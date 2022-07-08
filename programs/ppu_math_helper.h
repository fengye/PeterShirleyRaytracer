#ifndef _H_PPU_MATH_HELPER
#define _H_PPU_MATH_HELPER

#include <cmath>
#include <limits>


const double RT_infinity = std::numeric_limits<double>::infinity();
const double RT_pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * RT_pi / 180.0;
}

template<class T>
inline T clamp(const T v, const T min, const T max)
{
    return std::min(std::max(v, min), max);
}

#endif //_H_PPU_MATH_HELPER