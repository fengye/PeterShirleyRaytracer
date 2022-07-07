#include "spu_math_helper.h"


uint8_t clamp_uint8(const uint8_t v, const uint8_t min, const uint8_t max)
{
	return (v < min ? min : v) > max ? max : v;
}

FLOAT_TYPE clamp_float(const FLOAT_TYPE v, const FLOAT_TYPE min, const FLOAT_TYPE max)
{
	return (v < min ? min : v) > max ? max : v;
}
