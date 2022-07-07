#ifndef _H_SPHERE_
#define _H_SPHERE_

#ifdef _PPU_

#include "hittable.h"
#include "serialisable.h"

class sphere : public hittable, public serialisable
{
public:
	sphere() : centre(0, 0, 0), radius(0)
	{}

	sphere(const point3& c, double r) : centre(c.x(), c.y(), c.z()), radius(r)
	{}

	virtual bool hit(const ray& r, double tmin, double tmax, hit_record& record) const override;

	virtual void serialise(void* buf, uint8_t* out_size) const override;
	virtual void deserialise(const void* buf, uint8_t size) override;

public:
	point3 centre;
	double radius;
};

#endif

#ifdef _SPU_

#include "hittable.h"
#include "floattype.h"
#include "vec3.h"
#include "ray.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct _sphere_t
{
	point3_t centre;
	FLOAT_TYPE radius;
} sphere_t;

extern sphere_t sphere_create(const point3_t* c, FLOAT_TYPE r);
extern bool sphere_hit(const sphere_t* sphere, const ray_t* ray, FLOAT_TYPE tmin, FLOAT_TYPE tmax, hitrecord_t* out_record);
extern void sphere_serialise(const sphere_t* sphere, void* buf, uint8_t* out_size);
extern void sphere_deserialise(sphere_t* sphere, const void* buf, uint8_t size);

#endif

#endif  //_H_SPHERE_