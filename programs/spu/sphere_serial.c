#include "sphere.h"

void sphere_serialise(const sphere_t* sphere, void* buf, uint8_t* out_size)
{
	// explicitly use float
	FLOAT_TYPE* fbuf = (FLOAT_TYPE*)buf;
	fbuf[0] = (float)sphere->centre.e[0];
	fbuf[1] = (float)sphere->centre.e[1];
	fbuf[2] = (float)sphere->centre.e[2];
	fbuf[3] = (float)sphere->radius;

	*out_size = 4 * sizeof(float);
}
void sphere_deserialise(sphere_t* sphere, const void* buf, uint8_t size)
{
	const FLOAT_TYPE* fbuf = (const FLOAT_TYPE*)buf;
	sphere->centre.e[0] = fbuf[0];
	sphere->centre.e[1] = fbuf[1];
	sphere->centre.e[2] = fbuf[2];
	sphere->radius = fbuf[3];
}
