#ifndef _H_PPU_JOB_
#define _H_PPU_JOB_

#include <stdint.h>
#include <malloc.h>
#include "config.h"
#include "debug.h"
#include "spu_shared.h"
#include "job.h"
#include "tile.h"

struct ppu_job_t : public job_t
{
	world_data_t* input = nullptr;
	pixel_data_t* output = nullptr;
	uint32_t output_size = 0;
	uint32_t ppu_index = 0;

	ppu_job_t(int32_t img_width, int32_t img_height, int32_t start_tile_id, int32_t end_tile_id, uint32_t ppu_index_, void* job_data, uint32_t job_data_size)
	{
		input = (world_data_t*)memalign(SPU_ALIGN, sizeof(world_data_t));
		input->img_width = img_width;
		input->img_height = img_height;
		input->obj_data_ea = ptr2ea(job_data);
		input->obj_data_sz = job_data_size;
		input->start_tile_id = start_tile_id;
		input->end_tile_id = end_tile_id;
		input->focal_length = 1.0f;
		input->aspect_ratio = (float)img_width / (float)img_width;
		input->viewport_height = 2.0f;
		input->viewport_width = input->aspect_ratio * input->viewport_height;
		input->samples_per_pixel = SAMPLES_PER_PIXEL;
		input->max_bounce_depth = MAX_BOUNCE_DEPTH;

		int tileCount = end_tile_id - start_tile_id + 1;

		int pixelCount = TILE_WIDTH * TILE_HEIGHT * tileCount;
		output = (pixel_data_t*)memalign(SPU_ALIGN, pixelCount * sizeof(pixel_data_t));
		output_size = pixelCount * sizeof(pixel_data_t);

		ppu_index = ppu_index_;
	}

	virtual void print_job_details() const override
	{
		debug_printf("Input for PPU %d: tile %d->%d\n"
					"Output for PPU %d: %08x sizeof %d\n",
					ppu_index, input->start_tile_id, input->end_tile_id, 
					ppu_index, output, output_size);
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
		return (int8_t)ppu_index + SPU_COUNT;
	}

	~ppu_job_t()
	{
		free(output);
		free(input);
	}
};

#endif //_H_PPU_JOB_