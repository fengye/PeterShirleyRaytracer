#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <sys/spu_thread.h>

#define TAG 1

#include "spu_shared.h"
#include <malloc.h>
#include <stdlib.h>

#include "vec3.h"
#include "color_pixel_util.h"


#define nullptr NULL

#define SPU_ALIGN (16)
#define DMA_TRANSFER_LIMIT (16384)

/* The effective address of the input structure */
uint64_t spu_ea;
/* A copy of the structure sent by ppu */
spu_shared_t spu __attribute__((aligned(SPU_ALIGN)));

uint64_t world_data_ea;
world_data_t world_data __attribute__((aligned(SPU_ALIGN)));

uint64_t pixels_data_ea;
uint64_t pixels_data_size;
pixel_data_t* pixels_data = nullptr;

/* wait for dma transfer to be finished */
static void wait_for_completion(void) {
	mfc_write_tag_mask(1<<TAG);
	spu_mfcstat(MFC_TAG_UPDATE_ALL);
}

static void send_response() {
	spu.sync = 1;
	/* send sync to ppu variable with fence (this ensures sync is written AFTER response) */
	uint64_t ea = spu_ea + ((uint32_t)&spu.sync) - ((uint32_t)&spu);
	mfc_putf(&spu.sync, ea, 4, TAG, 0, 0);
}

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	/* get data structure */
	spu_ea = arg1;
	mfc_get(&spu, spu_ea, sizeof(spu_shared_t), TAG, 0, 0);
	wait_for_completion();

	/* get world data */
	world_data_ea = spu.i_data_ea;
	mfc_get(&world_data, world_data_ea, sizeof(world_data_t), TAG, 0, 0);
	wait_for_completion();

	/* get pixel output pointer */
	pixels_data_ea = spu.o_data_ea;
	pixels_data_size = spu.o_data_sz;
	pixels_data = (pixel_data_t*)memalign(SPU_ALIGN, pixels_data_size);

	/* blocking wait for signal notification */
	spu_read_signal1();

/*
	hittable_list world;
	world.add(std::make_shared<sphere>(point3(
		world_data.sphere_1.named.ox,
		world_data.sphere_1.named.oy,
		world_data.sphere_1.named.oz), world_data.sphere_1.named.radius ));
	world.add(std::make_shared<sphere>(point3(
		world_data.sphere_2.named.ox,
		world_data.sphere_2.named.oy,
		world_data.sphere_2.named.oz), world_data.sphere_2.named.radius ));

	camera cam;
*/
	for(int j = world_data.start_y; j < world_data.end_y; ++j)
	{
		for(int i = world_data.start_x; i < world_data.end_x; ++i)
		{
			double u = ((double)i) / (world_data.img_width - 1);
			double v = ((double)j) / (world_data.img_height - 1);

			int width = (world_data.end_x - world_data.start_x);
			int x = i - world_data.start_x;
			int y = j - world_data.start_y;

			color_t color = {{u, v, 0.25}};

			pixel_data_t* pixel = &pixels_data[y * width  + x];

			color_to_pixel_data(&color, pixel);
			/*
			color pixel_color;
			ray r = cam.get_ray(u, v);
			vec3 ray_dir = unit_vector(r.direction()); // because r.direction() is not normalized
			auto t = 0.5 * (ray_dir.y() + 1.0); // t <- [0, 1]
			pixel_color = (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); // white blend with blue graident
			*/
		}
	}

	/* write the result back */
	int32_t bytes_left = pixels_data_size;
	int32_t byte_written = 0;
	while(bytes_left > 0)
	{
		int32_t bytes_to_write = bytes_left > DMA_TRANSFER_LIMIT ? DMA_TRANSFER_LIMIT : bytes_left;
		mfc_put(((uint8_t*)pixels_data) + byte_written, pixels_data_ea + byte_written, bytes_to_write, TAG, 0, 0);
		bytes_left -= bytes_to_write;
		byte_written += bytes_to_write;
	}

	/* send the response message */
	send_response();
	wait_for_completion();

	/* properly exit the thread */
	spu_thread_exit(0);
	return 0;
}
