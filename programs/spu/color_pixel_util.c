#include "color_pixel_util.h"
#include "spu_math_helper.h"

pixel_data_t* color_to_pixel_data(const color_t* c, pixel_data_t* pixel_data)
{
	pixel_data->rgba[0] = clamp_uint8((uint8_t)(c->e[0] * 255.999), 0, 0xff);
	pixel_data->rgba[1] = clamp_uint8((uint8_t)(c->e[1] * 255.999), 0, 0xff);
	pixel_data->rgba[2] = clamp_uint8((uint8_t)(c->e[2] * 255.999), 0, 0xff);
	pixel_data->rgba[3] = 0xff;

	return pixel_data;
}