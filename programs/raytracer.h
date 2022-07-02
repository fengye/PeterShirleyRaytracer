#ifndef _H_RAYTRACER_
#define _H_RAYTRACER_

#include <cmath>
#include <limits>
#include <memory>

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

const double RT_infinity = std::numeric_limits<double>::infinity();
const double RT_pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * RT_pi / 180.0;
}

#include "ray.h"
#include "vec3.h"

#endif //_H_RAYTRACER_