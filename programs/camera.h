#ifndef _H_CAMERA_
#define _H_CAMERA_

#include "vec3.h"
#include "ray.h"
#include "floattype.h"

#ifdef _PPU_

class camera
{

public:
	camera()
	{
		aspect_ratio = 16.0 / 9.0;
		
		const FLOAT_TYPE viewport_height = 2.0;
		const FLOAT_TYPE viewport_width = viewport_height * aspect_ratio;
		const FLOAT_TYPE focal_length = 1.0;

		origin = point3(0, 0, 0);
		horizontal = vec3(viewport_width, 0, 0);
		vertical = vec3(0, viewport_height, 0);
		lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 + vec3(0, 0, -focal_length);
	}

	ray get_ray(FLOAT_TYPE u, FLOAT_TYPE v) const
	{
		return ray(origin, lower_left_corner + horizontal * u + vertical * v - origin);
	}

public:
	FLOAT_TYPE aspect_ratio;
	point3 origin;
	vec3 horizontal;
	vec3 vertical;
	vec3 lower_left_corner;
};

#endif

#ifdef _SPU_

typedef struct _camera_t 
{
	FLOAT_TYPE aspect_ratio;
	point3_t origin;
	vec3_t horizontal;
	vec3_t vertical;
	vec3_t lower_left_corner;
} camera_t;

extern camera_t camera_create(FLOAT_TYPE aspect_ratio, FLOAT_TYPE focal_length);
extern ray_t camera_get_ray(const camera_t* cam, FLOAT_TYPE u, FLOAT_TYPE v);

#endif

#endif //_H_CAMERA_