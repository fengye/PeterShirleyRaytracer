#ifndef _H_RAYTRACER_
#define _H_RAYTRACER_

#include <cmath>
#include <limits>
#include <memory>

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

template<class T>
inline T clamp(const T v, const T min, const T max)
{
    return std::min(std::max(v, min), max);
}

#include "ray.h"
#include "vec3.h"

#endif //_H_RAYTRACER_