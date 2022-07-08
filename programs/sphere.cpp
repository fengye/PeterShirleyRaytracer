#include "sphere.h"

bool sphere::hit(const ray& r, FLOAT_TYPE tmin, FLOAT_TYPE tmax, hit_record& record) const
{
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
}

void sphere::serialise(void* buf, uint8_t* out_size) const
{
	float* fbuf = (float*)buf;
	fbuf[0] = (float)centre.e[0];
	fbuf[1] = (float)centre.e[1];
	fbuf[2] = (float)centre.e[2];
	fbuf[3] = (float)radius;

	*out_size = 4 * sizeof(float);
}

void sphere::deserialise(const void* buf, uint8_t size)
{
	const float* fbuf = (const float*)buf;
	centre.e[0] = fbuf[0];
	centre.e[1] = fbuf[1];
	centre.e[2] = fbuf[2];
	radius = fbuf[3];
}
