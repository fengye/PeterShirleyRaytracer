#include "sphere.h"

bool sphere::hit(const ray& r, double tmin, double tmax, hit_record& record) const
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