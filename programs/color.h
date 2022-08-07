#ifndef _H_COLOR_
#define _H_COLOR_

#include <iostream>
#include "raytracer.h"
#include "bitmap.h"

extern void write_color(std::ostream& out, const color& pixel_color, int samples_per_pixel);
extern void write_color_bitmap(Bitmap* bitmap, int x, int y, const color& pixel_color, int samples_per_pixel);
extern void write_pixel_bitmap(Bitmap* bitmap, int x, int y, const pixel_data_t* pixel);

#endif //_H_COLOR_
