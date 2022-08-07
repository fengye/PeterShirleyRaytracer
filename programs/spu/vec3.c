#include "vec3.h"
#include "debug.h"
#include "string.h"
#include "random.h"
#include <stdbool.h>

#ifdef _SPU_

FLOAT_TYPE vec3_x(const vec3_t* v)
{
	return v->e[0];
}
FLOAT_TYPE vec3_y(const vec3_t* v)
{
	return v->e[1];
}
FLOAT_TYPE vec3_z(const vec3_t* v)
{
	return v->e[2];
}


vec3_t vec3_unit_vector(const vec3_t* v)
{
	FLOAT_TYPE len = vec3_length(v);
	vec3_t vv = {
		.e = {
			v->e[0] / len,
			v->e[1] / len,
			v->e[2] / len
		}
	};

	return vv;
}

vec3_t* vec3_normalise(vec3_t* v)
{
	FLOAT_TYPE len = vec3_length(v);
	return vec3_div(v, len);
}

FLOAT_TYPE vec3_dot(const vec3_t* u, const vec3_t* v)
{
	return u->e[0] * v->e[0] + u->e[1] * v->e[1] + u->e[2] * v->e[2];
}

vec3_t vec3_cross(const vec3_t* u, const vec3_t* v)
{
	// https://en.wikipedia.org/wiki/Cross_product
	// cross p = (a2b3 - a3b2)i, (a3b1 - a1b3)j, (a1b2 - a2b1)k
	vec3_t cr = {
		.e = {
				u->e[1] * v->e[2] - u->e[2] * v->e[1],
				u->e[2] * v->e[0] - u->e[0] * v->e[2],
				u->e[0] * v->e[1] - u->e[1] * v->e[0]
			}
		};

	return cr;
}

vec3_t vec3_create(FLOAT_TYPE x, FLOAT_TYPE y, FLOAT_TYPE z)
{
	vec3_t v = {{x, y, z}};
	return v;
}

vec3_t* vec3_assign(vec3_t* v, FLOAT_TYPE x, FLOAT_TYPE y, FLOAT_TYPE z)
{
	v->e[0] = x;
	v->e[1] = y;
	v->e[2] = z;
	return v;
}

vec3_t* vec3_assignv(vec3_t* v, const FLOAT_TYPE* xyz)
{
	memcpy(v->e, xyz, sizeof(FLOAT_TYPE) * 3);
	return v;
}

vec3_t  vec3_duplicate(const vec3_t* v)
{
	vec3_t vv = {
		.e = {v->e[0], v->e[1], v->e[2]}
	};
	return vv;
}

vec3_t* vec3_negate(vec3_t* v)
{
	v->e[0] *= -1;
	v->e[1] *= -1;
	v->e[2] *= -1;
	return v;
}

vec3_t* vec3_add(vec3_t* v, const vec3_t* rhs)
{
	v->e[0] += rhs->e[0];
	v->e[1] += rhs->e[1];
	v->e[2] += rhs->e[2];
	return v;
}

vec3_t* vec3_minus(vec3_t* v, const vec3_t* rhs)
{
	v->e[0] -= rhs->e[0];
	v->e[1] -= rhs->e[1];
	v->e[2] -= rhs->e[2];
	return v;
}

vec3_t* vec3_mul(vec3_t* v, FLOAT_TYPE t)
{
	v->e[0] *= t;
	v->e[1] *= t;
	v->e[2] *= t;
	return v;
}
vec3_t* vec3_mulv(vec3_t* v, const vec3_t* rhs)
{
	v->e[0] *= rhs->e[0];
	v->e[1] *= rhs->e[1];
	v->e[2] *= rhs->e[2];
	return v;
}
vec3_t* vec3_div(vec3_t* v, FLOAT_TYPE t)
{
	v->e[0] /= t;
	v->e[1] /= t;
	v->e[2] /= t;
	return v;
}
vec3_t* vec3_divv(vec3_t* v, const vec3_t* rhs)
{
	v->e[0] /= rhs->e[0];
	v->e[1] /= rhs->e[1];
	v->e[2] /= rhs->e[2];
	return v;
}

FLOAT_TYPE vec3_length_squared(const vec3_t* v)
{
	return v->e[0] * v->e[0] + v->e[1] * v->e[1] + v->e[2] * v->e[2];
}

FLOAT_TYPE vec3_length(const vec3_t* v)
{
	return sqrtf(vec3_length_squared(v));
}

vec3_t* vec3_random(vec3_t* v)
{
	v->e[0] = random_double();
	v->e[1] = random_double();
	v->e[2] = random_double();
	return v;
}
vec3_t* vec3_random_range(vec3_t* v, FLOAT_TYPE min, FLOAT_TYPE max)
{
	v->e[0] = random_double_range(min, max);
	v->e[1] = random_double_range(min, max);
	v->e[2] = random_double_range(min, max);
	return v;
}
vec3_t* vec3_random_in_unit_sphere(vec3_t* v)
{
	// First, pick a random point in the unit cube where x, y, and z all range from −1 to +1. 
	// Reject this point and try again if the point is outside the sphere.
	while(true)
	{
		if (vec3_length_squared(vec3_random_range(v, -1.0f, 1.0f)) > 1.0)
			continue;

		return v;
	}
}
vec3_t* vec3_random_unit_vector(vec3_t* v)
{
	return vec3_normalise(vec3_random_in_unit_sphere(v));
}
vec3_t* vec3_random_in_hemisphere(vec3_t* v, const vec3_t* normal)
{
	vec3_t* random_v = vec3_random_in_unit_sphere(v);
	if (vec3_dot(random_v, normal) > 0)
		return random_v;
	else
		return vec3_negate(random_v);
}

#endif
