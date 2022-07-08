#ifndef _H_HITTABLE_
#define _H_HITTABLE_

#ifdef _PPU_

#include "vec3.h"
#include "ray.h"
#include "floattype.h"

struct hit_record
{
	point3 p;
	vec3 normal;
	FLOAT_TYPE t;
	bool front_face;

	inline void set_face_normal(const ray& r, const vec3& outward_normal)
	{
		front_face = dot(r.direction(), outward_normal) < 0;
		normal = front_face ? outward_normal : -outward_normal;
	}
};

class hittable
{
public:
	virtual bool hit(const ray& r, FLOAT_TYPE tmin, FLOAT_TYPE tmax, hit_record& record) const = 0;
	
};

#endif


#ifdef _SPU_

#include "floattype.h"
#include "vec3.h"
#include "ray.h"
#include <stdbool.h>

typedef struct _hitrecord_t
{
	point3_t p;
	vec3_t normal;
	FLOAT_TYPE t;
	bool front_face;

} hitrecord_t;

extern void hitrecord_set_face_normal(hitrecord_t* record, const ray_t* r, const vec3_t* outward_normal);

#endif

#endif //_H_HITTABLE_