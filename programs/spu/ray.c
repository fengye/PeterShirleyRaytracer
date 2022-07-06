#include "ray.h"

ray_t* ray_assign(ray_t* r, const point3_t* orig, const vec3_t* dir)
{
	r->orig = *orig;
	r->dir = *dir;
	return r;
}

point3_t ray_at(const ray_t* r, FLOAT_TYPE t)
{
	point3_t o;
	vec3_assignv(&o, r->orig.e);

	vec3_t d;
	vec3_mul(vec3_assignv(&d, r->dir.e), t);

	vec3_add(&o, &d);

	// create
	return o;
}
