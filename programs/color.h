#ifndef _H_COLOR_
#define _H_COLOR_

#include <iostream>
#include "raytracer.h"
#include "vec3.h"

void write_color(std::ostream& out, const color& pixel_color, int samples_per_pixel)
{
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// Divide the color by the number of samples and gamma-correct for gamma=2.0.
	// For gamma 2.2 it should be power(color, 1/2.2)
	auto one_over_samples = 1.0 / samples_per_pixel;
	r = sqrt(r * one_over_samples);
	g = sqrt(g * one_over_samples);
	b = sqrt(b * one_over_samples);

	out << (int)(255.999 * clamp(r, 0.0, 0.999)) << ' ' 
		<< (int)(255.999 * clamp(g, 0.0, 0.999)) << ' ' 
		<< (int)(255.999 * clamp(b, 0.0, 0.999)) << '\n';
}


#endif //_H_COLOR_