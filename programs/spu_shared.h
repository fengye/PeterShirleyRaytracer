#ifndef _H_SPU_SHARED_
#define _H_SPU_SHARED_

#include "floattype.h"
#include <stdint.h>
#pragma pack(push)
#pragma pack(1)

#define SPU_COUNT (6)
#define SPU_ALIGN (16)

#define ptr2ea(x) ((u64)(void *)(x))

#define SYNC_START (0)
#define SYNC_FINISHED (1)
#define SYNC_QUIT (2)
#define SYNC_INVALID (0xFFFFFFFF)

// It looks like all structure needs to be multiple of 32-bytes in size
typedef struct _spu_shared
{
	uint32_t id;		/* spu thread id */
	uint32_t rank;		/* rank in SPU thread group (0..count-1) */
	uint32_t count;		/* number of threads in group */
	uint32_t sync;		/* sync value for response */
	uint32_t i_data_ea; /* effect address of input data */
	uint32_t i_data_sz;
	uint32_t o_data_ea; /* effect address of output data */
	uint32_t o_data_sz;
} spu_shared_t;

typedef struct _named_sphere_data
{
	float ox;
	float oy;
	float oz;
	float radius;
} named_sphere_data;

typedef union _sphere_data
{
	named_sphere_data named;
	float data[4];
} sphere_data_t;

typedef struct _world_data
{
	int32_t 			img_width; // 4
	int32_t 			img_height; // 4
	uint32_t            obj_data_ea; // 4
	uint32_t            obj_data_sz; // 4
	int32_t  			start_x; // 4
	int32_t  			start_y; // 4
	int32_t 			end_x; // 4
	int32_t 			end_y; // 4
	float  				viewport_width;	// 4
	float  				viewport_height; // 4
	float  				focal_length; // 4
	float  				aspect_ratio; // 4
	int32_t  			samples_per_pixel; // 4
	int32_t  			max_bounce_depth; // 4
	uint32_t            padding[2]; // 8
} world_data_t;

typedef struct _named_pixel_data
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} named_pixel_data;

typedef union _pixel_data
{
	named_pixel_data named;
	uint8_t rgba[4];
} pixel_data_t;

#pragma pack(pop)

#endif //_H_SPU_SHARED_