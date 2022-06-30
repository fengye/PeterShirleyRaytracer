#include <iostream>

#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "sphere.h"

double hit_sphere(const point3 centre, double radius, const ray& r)
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
		return -1.0;
	}
	else
	{
		//assume the closest hit point (smallest t), so we use [-sqrt(discriminant)] as the solution
		return (-HALF_B - std::sqrt(discriminant)) / A;
	}
}

color ray_color(const ray& r)
{
	sphere s(point3(0, 0, -1), 0.5);
	hit_record record;
	if (s.hit(r, 0, DBL_MAX, record))
	{
		return 0.5 * color(record.normal.x() + 1, record.normal.y() + 1, record.normal.z() + 1);
	}

	vec3 ray_dir = unit_vector(r.direction()); // because r.direction() is not normalized
	auto t = 0.5 * (ray_dir.y() + 1.0); // t <- [0, 1]
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); // white blend with blue graident
}

int main()
{
	// img
	const auto aspect_ratio = 16.0 / 9.0;
	const int img_width = 400;
	const int img_height = (int)(img_width / aspect_ratio);

	// camera
	const auto viewport_height = 2.0;
	const auto viewport_width = viewport_height * aspect_ratio;
	const auto focal_length = 1.0;

	const point3 origin(0.0, 0.0, 0.0);
	const vec3 horizontal(viewport_width, 0.0, 0.0);
	const vec3 vertical(0.0, viewport_height, 0.0);
	vec3 lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 + vec3(0, 0, -focal_length);

	std::cout << "P3\n" << img_width << ' ' << img_height << "\n255\n";

	for(int j = img_height-1; j >= 0; --j)
	{
		std::cerr << "\rScanline remaining: " << j << ' ' << std::flush;
		for(int i = 0; i < img_width; ++i)
		{
			double u = (double)i / (img_width - 1);
			double v = (double)j / (img_height - 1);

			color pixel_color = ray_color(ray(origin, lower_left_corner + u * horizontal + v * vertical - origin));
			write_color(std::cout, pixel_color);
		}
	}

	std::cerr << "\nDone.\n";
	return 0;
}