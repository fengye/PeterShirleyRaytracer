#ifndef _H_SPU_JOB_
#define _H_SPU_JOB_

#include <stdint.h>
#include <malloc.h>
#include "debug.h"
#include "spu_shared.h"
#include "job.h"
#include "tile.h"

struct spu_job_t : public job_t
{
	world_data_t* input = nullptr;
	pixel_data_t* output = nullptr;
	uint32_t output_size = 0;
	uint32_t spu_index = 0;

	spu_job_t(int32_t img_width, int32_t img_height, int global_start_tile_id, int global_end_tile_id, uint32_t spu_index_, void* job_data, uint32_t job_data_size)
	{
		input = (world_data_t*)memalign(SPU_ALIGN, sizeof(world_data_t));
		input->img_width = img_width;
		input->img_height = img_height;
		input->obj_data_ea = ptr2ea(job_data);
		input->obj_data_sz = job_data_size;
		input->global_start_tile_id = global_start_tile_id;
		input->global_end_tile_id = global_end_tile_id;
		input->focal_length = 1.0f;
		input->aspect_ratio = (float)img_width / (float)img_height;
		input->viewport_height = 2.0f;
		input->viewport_width = input->aspect_ratio * input->viewport_height;
		input->samples_per_pixel = SAMPLES_PER_PIXEL;
		input->max_bounce_depth = MAX_BOUNCE_DEPTH;

		int tileCount = global_end_tile_id - global_start_tile_id + 1;

		int pixelCount = TILE_WIDTH * TILE_HEIGHT * tileCount;
		output_size = pixelCount * sizeof(pixel_data_t);
		output = (pixel_data_t*)memalign(SPU_ALIGN, output_size);
		

		spu_index = spu_index_;
	}

	virtual void print_job_details() const override
	{
		debug_printf("Input for SPU %d: tile %d->%d\n"
					"Output for SPU %d: %08x sizeof %d\n",
					spu_index, input->global_start_tile_id, input->global_end_tile_id,
					spu_index, output, output_size);
	}

	virtual world_data_t* get_input() override {
		return input;
	}
	virtual pixel_data_t* get_output() override {
		return output;
	}

	virtual size_t get_output_size() const override {
		return output_size;
	}

	virtual int8_t get_processor_index() const override {
		return (int8_t)spu_index;
	}

	~spu_job_t()
	{
		free(output);
		free(input);
	}
};

#endif //_H_SPU_JOB_