#include "ppu_device.h"
#include "ppu_job.h"
#include "vec3.h"
#include "ray.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "ppu_math_helper.h"
#include "debug.h"
#include <memory>
#include <sys/cond.h>

static color ray_color(const ray& r, const hittable& world, int depth)
{
	if (depth < 0)
		return color(0, 0, 0);

	hit_record record;
	if (world.hit(r, 0.01f, RT_infinity, record))
	{
		//vec3 target = record.p + record.normal + vec3::random_in_unit_sphere();
		vec3 target = record.p + vec3::random_in_hemisphere(record.normal);
		return 0.5 * ray_color(ray(record.p, target-record.p), world, depth-1);
	}

	vec3 ray_dir = unit_vector(r.direction()); // because r.direction() is not normalized
	auto t = 0.5 * (ray_dir.y() + 1.0); // t <- [0, 1]
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); // white blend with blue graident
}


void ppu_raytrace(void* arg)
{
	ppu_device_t* ppu_device = (ppu_device_t*)arg;
	const char* threadname = ppu_device->threadname;
	debug_printf("PPU thread started: %s\n", threadname);

	while(true)
	{
		// WAIT_FOR_SIGNAL
		//debug_printf("%s waiting for signal\n", threadname);
		sysMutexLock(ppu_device->mutex, 0);
		sysCondWait(ppu_device->cond_var, 0);
		sysMutexUnlock(ppu_device->mutex);

		// IF SIGNALS QUIT, THEN BREAK THE LOOP
		if (ppu_device->sync == SYNC_QUIT)
		{
			debug_printf("%s received QUIT signal\n", threadname);
			break;
		}
		else
		{
			//debug_printf("%s received START signal\n", threadname);
			ppu_job_t* ppu_job = ppu_device->ppu_job;

			world_data_t* world_data = ppu_job->get_input();
			pixel_data_t* pixel_data = ppu_job->get_output();

			camera cam;
			const int samples_per_pixel = world_data->samples_per_pixel;
			const int max_depth = world_data->max_bounce_depth;

			uint8_t* obj_blob = ea2ptr(world_data->obj_data_ea);
			uint8_t sphere_size = sizeof(FLOAT_TYPE) * 4;
			const int32_t sphere_count = world_data->obj_data_sz / sphere_size;
			

//			debug_printf("%s sphere data %08x\n", threadname, obj_blob);
//			debug_printf("%s sphere count %d sphere type size: %d\n", threadname, sphere_count, sphere_size);
			hittable_list world;
			for(int i = 0; i < sphere_count; ++i)
			{
				auto sph = std::make_shared<sphere>();
				
				sph->deserialise(obj_blob + i * sphere_size, sphere_size);
				world.add(sph);
			}

//			debug_printf("%s start_y %d end_y %d start_x %d end_x %d\n", threadname, world_data->start_y, world_data->end_y, world_data->start_x, world_data->end_x);
//			debug_printf("%s samples_per_pixel %d max_depth %d\n", threadname, samples_per_pixel, max_depth);

			for(int j = world_data->start_y; j < world_data->end_y; ++j)
			{
				for(int i = world_data->start_x; i < world_data->end_x; ++i)
				{
					color pixel_color(0, 0, 0);

					int width = (world_data->end_x - world_data->start_x);
					int x = i - world_data->start_x;
					int y = j - world_data->start_y;

					for (int s = 0; s < samples_per_pixel; ++s) 
					{
						FLOAT_TYPE u = ((FLOAT_TYPE)i + random_double()) / (world_data->img_width - 1);
						FLOAT_TYPE v = ((FLOAT_TYPE)j + random_double()) / (world_data->img_height - 1);

						pixel_color += ray_color(cam.get_ray(u, v), world, max_depth);
		            }

		            // average by samples
		            pixel_color /= samples_per_pixel;

		            //debug_printf("%s Pixel (%d, %d)=(%.2f, %.2f, %.2f)\n", threadname, i, j, pixel_color.x(), pixel_color.y(), pixel_color.z());

					pixel_data_t* pixel = &pixel_data[y * width  + x];
					pixel_color.to_pixel_data(pixel);
					//debug_printf("%s Pixel output (%d, %d)=(%02x, %02x, %02x)\n", threadname, i, j, pixel->rgba[0], pixel->rgba[1], pixel->rgba[2]);
				}
			}

			// SEND FINISH SIGNAL
			sysMutexLock(ppu_device->mutex, 0);
			ppu_device->sync = SYNC_FINISHED;
			sysCondSignal(ppu_device->cond_var);
			sysMutexUnlock(ppu_device->mutex);
		}
	}

	sysThreadExit(0);
}