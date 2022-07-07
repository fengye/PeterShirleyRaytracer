#ifndef _H_VEC3_
#define _H_VEC3_

#include "floattype.h"
#include "random.h"

#ifdef _PPU_

#include <cmath>
#include <iostream>


using std::sqrt;

class vec3;
extern vec3 unit_vector(const vec3& v);
extern FLOAT_TYPE dot(const vec3& u, const vec3& v);
////////////////////////////////////////////////////////////////////
// vec3
class vec3
{
public:
	vec3() : e{0, 0 , 0}
	{}

	vec3(FLOAT_TYPE e0, FLOAT_TYPE e1, FLOAT_TYPE e2) : e{e0, e1, e2}
	{}

	FLOAT_TYPE x() const {return e[0];}
	FLOAT_TYPE y() const {return e[1];}
	FLOAT_TYPE z() const {return e[2];}

	vec3 operator-() const
	{
		return vec3(-e[0], -e[1], -e[2]);
	}
	FLOAT_TYPE operator[](int i) const
	{
		return e[i];
	}
	FLOAT_TYPE& operator[](int i)
	{
		return e[i];
	}

	vec3& operator+=(const vec3& rhs)
	{
		e[0] += rhs.e[0];
		e[1] += rhs.e[1];
		e[2] += rhs.e[2];
		return *this;
	}

	vec3& operator*=(const FLOAT_TYPE t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator/=(const FLOAT_TYPE t)
	{
		return (*this) *= 1/t;
	}

	FLOAT_TYPE length_squared() const
	{
		return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
	}

	FLOAT_TYPE length() const
	{
		return sqrt(length_squared());
	}

	static vec3 random()
	{
		return vec3(random_double(), random_double(), random_double());
	}

	static vec3 random(FLOAT_TYPE min, FLOAT_TYPE max)
	{
		return vec3(random_double_range(min, max), random_double_range(min, max), random_double_range(min, max));
	}

	static vec3 random_in_unit_sphere()
	{
		// First, pick a random point in the unit cube where x, y, and z all range from −1 to +1. 
		// Reject this point and try again if the point is outside the sphere.
		while(true)
		{
			vec3 v = random(-1.0, 1.0);
			if (v.length_squared() > 1.0)
				continue;

			return v;
		}
	}

	static vec3 random_unit_vector()
	{
		return unit_vector(random_in_unit_sphere());
	}

	static vec3 random_in_hemisphere(const vec3& normal)
	{
		vec3 random_vec = random_in_unit_sphere();
		if (dot(random_vec, normal) > 0)
			return random_vec;
		else
			return -random_vec;
	}


public:
	FLOAT_TYPE e[3];	
};

using point3 = vec3;
using color = vec3;

//////////////////////////////////////////////////////////////////
// vec3 global operator overriding
inline std::ostream& operator<<(std::ostream& out, const vec3& v)
{
	return out << v[0] << ' ' << v[1] << ' ' << v[2];
}

inline vec3 operator+(const vec3& lhs, const vec3& rhs)
{
	return vec3(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}

inline vec3 operator-(const vec3& lhs, const vec3& rhs)
{
	return vec3(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}

inline vec3 operator*(const FLOAT_TYPE t, const vec3& rhs)
{
	return vec3(t*rhs[0], t*rhs[1], t*rhs[2]);
}

inline vec3 operator*(const vec3& lhs, const vec3& rhs)
{
	return vec3(lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}

inline vec3 operator*(const vec3& lhs, const FLOAT_TYPE t)
{
	return t * lhs;
}

inline vec3 operator/(const vec3& lhs, const FLOAT_TYPE t)
{
	return (1/t) * lhs;
}

inline FLOAT_TYPE dot(const vec3& u, const vec3& v)
{
	return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

inline vec3 cross(const vec3& u, const vec3& v)
{
	// https://en.wikipedia.org/wiki/Cross_product
	// cross p = (a2b3 - a3b2)i, (a3b1 - a1b3)j, (a1b2 - a2b1)k
	return vec3(
			u[1] * v[2] - u[2] * v[1],
			u[2] * v[0] - u[0] * v[2],
			u[0] * v[1] - u[1] * v[0]
		);
}

inline vec3 unit_vector(const vec3& v)
{
	return v / v.length();
}

#endif

#ifdef _SPU_

#include <math.h>
#include <random.h>

typedef struct _vec3_t
{
	FLOAT_TYPE e[3];
} vec3_t;

typedef vec3_t point3_t;
typedef vec3_t color_t;

extern vec3_t vec3_unit_vector(const vec3_t* v);
extern vec3_t* vec3_normalise(vec3_t* v);
extern FLOAT_TYPE vec3_dot(const vec3_t* u, const vec3_t* v);
extern vec3_t vec3_cross(const vec3_t* u, const vec3_t* v);

extern FLOAT_TYPE vec3_x(const vec3_t* v);
extern FLOAT_TYPE vec3_y(const vec3_t* v);
extern FLOAT_TYPE vec3_z(const vec3_t* v);
extern vec3_t  vec3_create(FLOAT_TYPE x, FLOAT_TYPE y, FLOAT_TYPE z);
extern vec3_t* vec3_assign(vec3_t* v, FLOAT_TYPE x, FLOAT_TYPE y, FLOAT_TYPE z);
extern vec3_t* vec3_assignv(vec3_t* v, const FLOAT_TYPE* xyz);
extern vec3_t  vec3_duplicate(const vec3_t* v);
extern vec3_t* vec3_negate(vec3_t* v);
extern vec3_t* vec3_add(vec3_t* v, const vec3_t* rhs);
extern vec3_t* vec3_minus(vec3_t* v, const vec3_t* rhs);
extern vec3_t* vec3_mul(vec3_t* v, FLOAT_TYPE t);
extern vec3_t* vec3_mulv(vec3_t* v, const vec3_t* rhs);
extern vec3_t* vec3_div(vec3_t* v, FLOAT_TYPE t);
extern vec3_t* vec3_divv(vec3_t* v, const vec3_t* rhs);
extern FLOAT_TYPE vec3_length_squared(const vec3_t* v);
extern FLOAT_TYPE vec3_length(const vec3_t* v);

extern vec3_t* vec3_random(vec3_t* v);
extern vec3_t* vec3_random_range(vec3_t* v, FLOAT_TYPE min, FLOAT_TYPE max);
extern vec3_t* vec3_random_in_unit_sphere(vec3_t* v);
extern vec3_t* vec3_random_unit_vector(vec3_t* v);
extern vec3_t* vec3_random_in_hemisphere(vec3_t* v, const vec3_t* normal);

#endif

#endif //_H_VEC3_