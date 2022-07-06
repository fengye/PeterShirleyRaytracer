#ifndef _H_RAY_
#define _H_RAY_

#include "vec3.h"
#include "floattype.h"

#ifdef _PPU_

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

	point3 at(FLOAT_TYPE t) const
	{
		return orig + dir * t;
	}

public:
	point3 orig;
	vec3 dir;
};

#endif

#ifdef _SPU_

typedef struct _ray_t {
	point3_t orig;
	vec3_t dir;
} ray_t;

extern ray_t* ray_assign(ray_t* r, const point3_t* orig, const vec3_t* dir);
extern point3_t ray_at(const ray_t* r, FLOAT_TYPE t);

#endif

#endif // _H_RAY_