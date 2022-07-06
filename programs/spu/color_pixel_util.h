#ifndef _H_COLOR_PIXEL_UTILITY
#define _H_COLOR_PIXEL_UTILITY

#include "vec3.h"

#ifdef _SPU_
	
#include "spu_shared.h"

pixel_data_t* color_to_pixel_data(const color_t* c, pixel_data_t* pixel_data);

#endif

#endif //_H_COLOR_PIXEL_UTILITY