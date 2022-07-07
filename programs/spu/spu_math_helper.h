#ifndef _H_SPU_MATH_HELPER_
#define _H_SPU_MATH_HELPER_

#include <stdint.h>
#include "floattype.h"

extern uint8_t clamp_uint8(const uint8_t v, const uint8_t min, const uint8_t max);
extern FLOAT_TYPE clamp_float(const FLOAT_TYPE v, const FLOAT_TYPE min, const FLOAT_TYPE max);

#endif //_H_SPU_MATH_HELPER_