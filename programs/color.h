#ifndef _H_COLOR_
#define _H_COLOR_

#include <iostream>
#include "vec3.h"

void write_color(std::ostream& out, const color& pixel_color)
{
	out << (int)(255.999 * pixel_color.x()) << ' ' 
		<< (int)(255.999 * pixel_color.y()) << ' ' 
		<< (int)(255.999 * pixel_color.z()) << '\n';
}


#endif //_H_COLOR_