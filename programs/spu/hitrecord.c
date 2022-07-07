#include "hittable.h"

void hitrecord_set_face_normal(hitrecord_t* record, const ray_t* r, const vec3_t* outward_normal)
{
	record->front_face = vec3_dot(&r->dir, outward_normal) < 0;
	record->normal = *outward_normal;
	if (!record->front_face)
	{
		vec3_negate(&record->normal);
	}
}