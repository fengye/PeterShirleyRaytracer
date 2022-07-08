#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include <sys/spu_thread.h>

#define TAG 1

#include "spu_shared.h"
#include <malloc.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vec3.h"
#include "ray.h"
#include "color_pixel_util.h"
#include "sphere.h"
#include "camera.h"

#define nullptr NULL

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

static void send_response(uint32_t code) {
	spu.sync = code;
	/* send sync to ppu variable with fence (this ensures sync is written AFTER response) */
	uint64_t ea = spu_ea + ((uint32_t)&spu.sync) - ((uint32_t)&spu);
	mfc_putf(&spu.sync, ea, 4, TAG, 0, 0);
}

static bool world_hit(sphere_t** sphere_list, int32_t sphere_count, const ray_t* r, FLOAT_TYPE tmin, FLOAT_TYPE tmax, hitrecord_t* out_record)
{
	hitrecord_t tmp_record;
	bool hit_anything = false;
	FLOAT_TYPE closest_so_far = tmax;

	for(int i = 0; i < sphere_count; ++i)
	{
		if (sphere_hit(sphere_list[i], r, tmin, closest_so_far, &tmp_record))
		{
			hit_anything = true;
			closest_so_far = tmp_record.t;
			*out_record = tmp_record;
		}
	}
	
	return hit_anything;
}


// Material function
static color_t ray_color(sphere_t** sphere_list, int32_t sphere_count, const ray_t* r, int depth) {

	if (depth <= 0)
	{
        color_t black = {{0, 0, 0}};
        return black;
	}

	hitrecord_t rec;
    if (world_hit(sphere_list, sphere_count, r, 0.001f, INFINITY, &rec)) {

    	vec3_t random;
    	//vec3_random_in_unit_sphere(&random);
    	//vec3_random_unit_vector(&random);
    	vec3_random_in_hemisphere(&random, &rec.normal);

    	point3_t target = vec3_duplicate(&rec.p);
    	//vec3_add(&target, &rec.normal);
    	vec3_add(&target, &random);

    	ray_t secondary_ray;
    	vec3_t target_dir = vec3_duplicate(&target);
    	vec3_minus(&target_dir, &rec.p);

    	ray_assign(&secondary_ray, &rec.p, &target_dir);

    	const FLOAT_TYPE reflection_rate = 0.5f;
    	color_t secondary_ray_color = ray_color(sphere_list, sphere_count, &secondary_ray, depth-1);
    	vec3_mul(&secondary_ray_color, reflection_rate);
        return secondary_ray_color;
    }

    vec3_t unit_direction = vec3_unit_vector(&r->dir);
    FLOAT_TYPE t = 0.5f * (vec3_y(&unit_direction) + 1.0f);
    
    color_t white = {{1.0f, 1.0f, 1.0f}};
    color_t blue = {{0.5f, 0.7f, 1.0f}};

    vec3_add(vec3_mul(&white, 1.0f - t), vec3_mul(&blue, t));
    return white;
}

int main(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	/* get data structure */
	spu_ea = arg1;
	while(true)
	{
		/* blocking wait for signal notification */
		spu_read_signal1();

		mfc_get(&spu, spu_ea, sizeof(spu_shared_t), TAG, 0, 0);
		wait_for_completion();

		if (spu.sync == SYNC_QUIT)
		{
			break;
		}
		else
		{
			/* get world data */
			world_data_ea = spu.i_data_ea;
			mfc_get(&world_data, world_data_ea, sizeof(world_data_t), TAG, 0, 0);
			wait_for_completion();

			// get world actual obj data
			uint8_t* obj_blob = memalign(SPU_ALIGN, world_data.obj_data_sz);
			mfc_get(obj_blob, world_data.obj_data_ea, world_data.obj_data_sz, TAG, 0, 0);
			wait_for_completion();

			const int32_t sphere_type_size = sizeof(sphere_t);
			const int32_t sphere_count = world_data.obj_data_sz / sphere_type_size;
			

			// sphere list
			sphere_t** spheres = (sphere_t**)memalign(SPU_ALIGN, sizeof(sphere_t*) * sphere_count);
			for(int i = 0; i < sphere_count; ++i)
			{
				sphere_t* sphere = (sphere_t*)memalign(SPU_ALIGN, sphere_type_size);
				sphere_deserialise(sphere, obj_blob + i * sphere_type_size, sphere_type_size);

				spheres[i] = sphere;
			}

			// all deserialised, no longer use the blob
			free(obj_blob);

			/* get pixel output pointer */
			pixels_data_ea = spu.o_data_ea;
			pixels_data_size = spu.o_data_sz;
			pixels_data = (pixel_data_t*)memalign(SPU_ALIGN, pixels_data_size);

			const int samples_per_pixel = world_data.samples_per_pixel;
			const int max_depth = world_data.max_bounce_depth;

			camera_t cam = camera_create(world_data.aspect_ratio, world_data.focal_length);

			for(int j = world_data.start_y; j < world_data.end_y; ++j)
			{
				for(int i = world_data.start_x; i < world_data.end_x; ++i)
				{
					int width = (world_data.end_x - world_data.start_x);
					int x = i - world_data.start_x;
					int y = j - world_data.start_y;

					color_t color = vec3_create(0, 0, 0);
					for (int s = 0; s < samples_per_pixel; ++s) {

						FLOAT_TYPE u = ((FLOAT_TYPE)i + random_float()) / (world_data.img_width - 1);
						FLOAT_TYPE v = ((FLOAT_TYPE)j + random_float()) / (world_data.img_height - 1);

		                ray_t r = camera_get_ray(&cam, u, v);
		                color_t color_contrib = ray_color(spheres, sphere_count, &r, max_depth);
		                vec3_add(&color, &color_contrib);
		            }

		            // average by samples
		            vec3_div(&color, (FLOAT_TYPE)samples_per_pixel);
		            // approximate gamma correction
#if 0
		            color.e[0] = sqrtf(color.e[0]);
		            color.e[1] = sqrtf(color.e[1]);
		            color.e[2] = sqrtf(color.e[2]);
#endif

					pixel_data_t* pixel = &pixels_data[y * width  + x];
					color_to_pixel_data(&color, pixel);
				}
			}

			
			// free up sphere and sphere list memory
			for(int i = 0; i < sphere_count; ++i)
			{
				free(spheres[i]);
			}

			free(spheres);

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
			send_response(SYNC_FINISHED);
			wait_for_completion();

			free(pixels_data);
		}
	}

	/* properly exit the thread */
	spu_thread_exit(0);
	return 0;
}
