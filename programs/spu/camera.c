#include "camera.h"

camera_t camera_create(FLOAT_TYPE aspect_ratio, FLOAT_TYPE focal_length)
{
	const FLOAT_TYPE viewport_height = 2.0;
	const FLOAT_TYPE viewport_width = viewport_height * aspect_ratio;

	/*
	origin = point3(0, 0, 0);
	horizontal = vec3(viewport_width, 0, 0);
	vertical = vec3(0, viewport_height, 0);
	lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 + vec3(0, 0, -focal_length);
	*/

	camera_t c = {
		.aspect_ratio = aspect_ratio,
		.origin = {{0, 0, 0}},
		.horizontal = {{viewport_width, 0, 0}},
		.vertical = {{0, viewport_height, 0}}
	};

	
	vec3_t half_horz = vec3_duplicate(&c.horizontal);
	vec3_div(&half_horz, 2.0f);
	vec3_t half_vert = vec3_duplicate(&c.vertical);
	vec3_div(&half_vert, 2.0f);

	vec3_t focal_offset = vec3_create(0, 0, -focal_length);

	c.lower_left_corner = vec3_duplicate(&c.origin);
	vec3_minus(&c.lower_left_corner, &half_horz);
	vec3_minus(&c.lower_left_corner, &half_vert);
	vec3_add(&c.lower_left_corner, &focal_offset);

	return c;
}

ray_t camera_get_ray(const camera_t* cam, FLOAT_TYPE u, FLOAT_TYPE v)
{
	//return ray(origin, lower_left_corner + horizontal * u + vertical * v - origin);
	ray_t ray;

	vec3_t horz_dir;
	vec3_t vert_dir;
	vec3_t dir;

	vec3_mul(vec3_assignv(&horz_dir, cam->horizontal.e), u);
	vec3_mul(vec3_assignv(&vert_dir, cam->vertical.e), v);
	vec3_minus(vec3_add(vec3_add(vec3_assignv(&dir, cam->lower_left_corner.e), &horz_dir), &vert_dir), &cam->origin);

	ray_assign(&ray, &cam->origin, &dir);

	return ray;
}
