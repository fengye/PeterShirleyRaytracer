#ifndef _H_VEC3_
#define _H_VEC3_

#include <cmath>
#include <iostream>

using std::sqrt;

////////////////////////////////////////////////////////////////////
// vec3
class vec3
{
public:
	vec3() : e{0, 0 , 0}
	{}

	vec3(double e0, double e1, double e2) : e{e0, e1, e2}
	{}

	double x() const {return e[0];}
	double y() const {return e[1];}
	double z() const {return e[2];}

	vec3 operator-() const
	{
		return vec3(-e[0], -e[1], -e[2]);
	}
	double operator[](int i) const
	{
		return e[i];
	}
	double& operator[](int i)
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

	vec3& operator*=(const double t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator/=(const double t)
	{
		return (*this) *= 1/t;
	}

	double length_squared() const
	{
		return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
	}

	double length() const
	{
		return sqrt(length_squared());
	}


public:
	double e[3];	
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

inline vec3 operator*(const double t, const vec3& rhs)
{
	return vec3(t*rhs[0], t*rhs[1], t*rhs[2]);
}

inline vec3 operator*(const vec3& lhs, const vec3& rhs)
{
	return vec3(lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}

inline vec3 operator*(const vec3& lhs, const double t)
{
	return t * lhs;
}

inline vec3 operator/(const vec3& lhs, const double t)
{
	return (1/t) * lhs;
}

inline double dot(const vec3& u, const vec3& v)
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

#endif //_H_VEC3_