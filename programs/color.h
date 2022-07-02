#ifndef _H_COLOR_
#define _H_COLOR_

#include "debug.h"
#include "vec3.h"
#include "bitmap.h"

void write_color(const color& pixel_color)
{
	debug_printf("%d %d %d\n", 
		(int)(255.999 * pixel_color.x()),
		(int)(255.999 * pixel_color.y()),
		(int)(255.999 * pixel_color.z()));
	/*
	out << (int)(255.999 * pixel_color.x()) << ' ' 
		<< (int)(255.999 * pixel_color.y()) << ' ' 
		<< (int)(255.999 * pixel_color.z()) << '\n';
	*/
}

void write_color_bitmap(Bitmap* bitmap, int x, int y, const color& pixel_color)
{
	bitmapUpdateRGBA(bitmap, (u32)x, (u32)y, 
		(u8)(255.999 * pixel_color.x()),
		(u8)(255.999 * pixel_color.y()),
		(u8)(255.999 * pixel_color.z()), 0xff);
}


#endif //_H_COLOR_