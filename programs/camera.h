#ifndef _H_CAMERA_
#define _H_CAMERA_

#include "vec3.h"
#include "ray.h"

class camera
{

public:
	camera()
	{
		aspect_ratio = 16.0 / 9.0;
		
		const double viewport_height = 2.0;
		const double viewport_width = viewport_height * aspect_ratio;
		const double focal_length = 1.0;

		origin = point3(0, 0, 0);
		horizontal = vec3(viewport_width, 0, 0);
		vertical = vec3(0, viewport_height, 0);
		lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 + vec3(0, 0, -focal_length);
	}

	ray get_ray(double u, double v) const
	{
		return ray(origin, lower_left_corner + horizontal * u + vertical * v - origin);
	}

public:
	double aspect_ratio;
	point3 origin;
	vec3 horizontal;
	vec3 vertical;
	vec3 lower_left_corner;
};

#endif //_H_CAMERA_