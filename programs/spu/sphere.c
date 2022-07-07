#include "sphere.h"
#include <math.h>

sphere_t sphere_create(const point3_t* c, FLOAT_TYPE r)
{
	sphere_t sphere = {
		.centre = {{c->e[0], c->e[1], c->e[2]}},
		.radius = r
	};

	return sphere;
}

bool sphere_hit(const sphere_t* sphere, const ray_t* r, FLOAT_TYPE tmin, FLOAT_TYPE tmax, hitrecord_t* out_record)
{
	/*
	// To find t of the ray = A + tb
	auto b = r.direction();
	auto a_minus_c = r.origin() - centre;

	auto A = dot(b, b);
	auto HALF_B = dot(b, a_minus_c);
	auto C = dot(a_minus_c, a_minus_c) - radius * radius;
	// If there's real number solution, the discriminant needs to be greater than or equal to zero
	// B^2 - 4AC
	auto discriminant = (HALF_B * HALF_B - A * C);
	if (discriminant < 0)
	{
		return false;
	}
	else
	{
		// Find the nearest root that lies in the acceptable range.
		//assume the closest hit point (smallest t), so we use [-sqrt(discriminant)] as the solution
		auto sqrt_d = std::sqrt(discriminant);
		auto t = (-HALF_B - sqrt_d) / A;

		if (t < tmin || t > tmax)
		{
			// try another root
			t = (-HALF_B + sqrt_d) / A;
			if (t < tmin || t > tmax)
				return false;
		}

		record.p = r.at(t);
		record.set_face_normal(r, (record.p - centre) / radius);
		record.t = t;
		
		return true;
	}
	*/
	vec3_t b = r->dir;
	vec3_t a_minus_c = vec3_duplicate(&r->orig);
	vec3_minus(&a_minus_c, &sphere->centre);

	FLOAT_TYPE A = vec3_dot(&b, &b);
	FLOAT_TYPE HALF_B = vec3_dot(&b, &a_minus_c);
	FLOAT_TYPE C = vec3_dot(&a_minus_c, &a_minus_c) - sphere->radius * sphere->radius;
	// If there's real number solution, the discriminant needs to be greater than or equal to zero
	// B^2 - 4AC
	FLOAT_TYPE discriminant = (HALF_B*HALF_B - A*C);
	if (discriminant < 0)
	{
		return false;
	}
	else
	{
		// Find the nearest root that lies in the acceptable range.
		//assume the closest hit point (smallest t), so we use [-sqrt(discriminant)] as the solution
		FLOAT_TYPE sqrt_d = sqrtf(discriminant);
		FLOAT_TYPE t = (-HALF_B - sqrt_d) / A;

		if (t < tmin || t > tmax)
		{
			t = (-HALF_B + sqrt_d) / A;
			if (t < tmin || t > tmax)
				return false;
		}

		out_record->p = ray_at(r, t);
		out_record->t = t;

		vec3_t outward_normal = vec3_duplicate(&out_record->p);
		vec3_minus(&outward_normal, &sphere->centre);
		vec3_div(&outward_normal, sphere->radius);
		hitrecord_set_face_normal(out_record, r, &outward_normal);

		return true;
	}
}