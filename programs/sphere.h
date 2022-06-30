#ifndef _H_SPHERE_
#define _H_SPHERE_

#include "hittable.h"

class sphere : public hittable
{
public:
	sphere() : centre(0, 0, 0), radius(0)
	{}

	sphere(const point3& c, double r) : centre(c.x(), c.y(), c.z()), radius(r)
	{}

	virtual bool hit(const ray& r, double tmin, double tmax, hit_record& record) const override;

public:
	point3 centre;
	double radius;
};

#endif  //_H_SPHERE_