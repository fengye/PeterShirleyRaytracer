#include "color.h"
#include "debug.h"
#include "vec3.h"
#include "spu_shared.h"

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

	debug_printf("%d %d %d\n", 
		(int)(255.999 * clamp<FLOAT_TYPE>(r, 0.0, 0.999)),
		(int)(255.999 * clamp<FLOAT_TYPE>(g, 0.0, 0.999)),
		(int)(255.999 * clamp<FLOAT_TYPE>(b, 0.0, 0.999)));
}

void write_color_bitmap(Bitmap* bitmap, int x, int y, const color& pixel_color, int samples_per_pixel)
{

	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	auto one_over_samples = 1.0 / samples_per_pixel;
	r *= one_over_samples;
	g *= one_over_samples;
	b *= one_over_samples;

	bitmapUpdateRGBA(bitmap, (u32)x, (u32)y, 
		(u8)(255.999 * clamp<FLOAT_TYPE>(r, 0.0, 0.999)),
		(u8)(255.999 * clamp<FLOAT_TYPE>(g, 0.0, 0.999)),
		(u8)(255.999 * clamp<FLOAT_TYPE>(b, 0.0, 0.999)), 0xff);
}

void write_pixel_bitmap(Bitmap* bitmap, int x, int y, const pixel_data_t* pixel)
{
	bitmapUpdateRGBA(bitmap, (u32)x, (u32)y, 
		pixel->rgba[0], pixel->rgba[1], pixel->rgba[2], pixel->rgba[3]);
}
