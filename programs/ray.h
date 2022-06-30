#ifndef _H_RAY_
#define _H_RAY_

#include "vec3.h"

class ray {

public:
	ray() {}

	ray(const point3& orig_, const vec3& dir_)
	: orig(orig_), dir(dir_)
	{}

	point3 origin() const
	{
		return orig;
	}

	vec3 direction() const
	{
		return dir;
	}

	point3 at(double t) const
	{
		return orig + dir * t;
	}

public:
	point3 orig;
	vec3 dir;
};

#endif // _H_RAY_