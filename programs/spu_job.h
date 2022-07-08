#ifndef _H_SPU_JOB_
#define _H_SPU_JOB_

#include <stdint.h>
#include "debug.h"
#include "spu_shared.h"

struct spu_job_t
{
	world_data_t* input = nullptr;
	pixel_data_t* output = nullptr;
	uint32_t output_size = 0;
	uint32_t spu_index = 0;

	spu_job_t(int32_t img_width, int32_t img_height, int32_t start_y, int32_t end_y, uint32_t spu_index_, void* job_data, uint32_t job_data_size)
	{
		input = (world_data_t*)memalign(SPU_ALIGN, sizeof(world_data_t));
		input->img_width = img_width;
		input->img_height = img_height;
		input->obj_data_ea = ptr2ea(job_data);
		input->obj_data_sz = job_data_size;
		input->start_x = 0;
		input->end_x = img_width;
		input->start_y = start_y;
		input->end_y = end_y;
		input->focal_length = 1.0f;
		input->aspect_ratio = 16.0f / 9.0f;
		input->viewport_height = 2.0f;
		input->viewport_width = input->aspect_ratio * input->viewport_height;
		input->samples_per_pixel = SAMPLES_PER_PIXEL;
		input->max_bounce_depth = MAX_BOUNCE_DEPTH;

		int pixelCount = (input->end_x - input->start_x) * (input->end_y - input->start_y);
		output = (pixel_data_t*)memalign(SPU_ALIGN, pixelCount * sizeof(pixel_data_t));
		output_size = pixelCount * sizeof(pixel_data_t);

		spu_index = spu_index_;
	}

	void print_spu_job() const
	{
		debug_printf("Input for SPU %d: %d->%d, %d->%d\n"
					"Output for SPU %d: %08x sizeof %d\n",
					spu_index, input->start_x, input->end_x, input->start_y, input->end_y,
					spu_index, output, output_size);
	}

	~spu_job_t()
	{
		free(output);
		free(input);
	}
};

#endif //_H_SPU_JOB_