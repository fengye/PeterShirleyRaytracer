#ifndef _H_HITTABLE_
#define _H_HITTABLE_

#include "vec3.h"
#include "ray.h"

struct hit_record
{
	point3 p;
	vec3 normal;
	double t;
};

class hittable
{
public:
	virtual bool hit(const ray& r, double tmin, double tmax, hit_record& record) const = 0;
	
};

#endif //_H_HITTABLE_