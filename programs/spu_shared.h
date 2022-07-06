#ifndef _H_SPU_SHARED_
#define _H_SPU_SHARED_

#pragma pack(push)
#pragma pack(1)

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
	int32_t 			img_width; // 8
	int32_t 			img_height; // 8
	sphere_data_t		sphere_1; // 16
	sphere_data_t		sphere_2; // 16
	int32_t  			start_x; // 8
	int32_t  			start_y; // 8
	int32_t 			end_x; // 8
	int32_t 			end_y; // 8
	uint32_t            padding[2]; // 16
} world_data_t;

typedef struct _named_pixel_data
{
	float r;
	float g;
	float b;
	float a;
} named_pixel_data;

typedef union _pixel_data
{
	named_pixel_data named;
	float rgba[4];
} pixel_data_t;

#pragma pack(pop)

#endif //_H_SPU_SHARED_