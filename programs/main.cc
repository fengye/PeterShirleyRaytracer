#include <iostream>
#include "raytracer.h"

#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"

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

color ray_color(const ray& r, const hittable& world)
{
	hit_record record;
	if (world.hit(r, 0, infinity, record))
	{
		return 0.5 * (record.normal + color(1, 1, 1));
	}

	vec3 ray_dir = unit_vector(r.direction()); // because r.direction() is not normalized
	auto t = 0.5 * (ray_dir.y() + 1.0); // t <- [0, 1]
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); // white blend with blue graident
}

int main()
{
	// camera
	camera cam;

	// img
	const int img_width = 400;
	const int img_height = (int)(img_width / cam.aspect_ratio);

	// world
	hittable_list world;
	world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
	world.add(make_shared<sphere>(point3(0, -100.5, 0), 100.0));

	// multisample
	const int sample_per_pixel = 100;

	std::cout << "P3\n" << img_width << ' ' << img_height << "\n255\n";

	for(int j = img_height-1; j >= 0; --j)
	{
		std::cerr << "\rScanline remaining: " << j << ' ' << std::flush;
		for(int i = 0; i < img_width; ++i)
		{
			color pixel_color(0, 0, 0);
			for (int s = 0; s < sample_per_pixel; ++s)
			{
				double u = ((double)i + random_double()) / (img_width - 1);
				double v = ((double)j + random_double()) / (img_height - 1);

				pixel_color += ray_color(cam.get_ray(u, v), world);
			}

			write_color(std::cout, pixel_color, sample_per_pixel);
		}
	}

	std::cerr << "\nDone.\n";
	return 0;
}