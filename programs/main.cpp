#include "debug.h"

#include "raytracer.h"
#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"
#include <cstdio>
#include <malloc.h>
#include <unistd.h>
#include <cfloat>
#include <vector>
// PS3 specific
#include <io/pad.h>
#include <rsx/rsx.h>
#include <sysutil/sysutil.h>
#include <sys/process.h>
// ~PS3 specific
#include "bitmap.h"
#include "rsxutil.h"
#include <sys/time.h>
#include <assert.h>

#define STOP_SPU_BY_TERMINATION 0

double hit_sphere(const point3 centre, double radius, const ray& r)
{
	// To find t of the ray = A + tb
	auto b = r.direction();
	auto a_minus_c = r.origin() - centre;

	auto A = dot(b, b);
	auto HALF_B = dot(b, a_minus_c);
	auto C = dot(a_minus_c, a_minus_c) - radius * radius;
	// If there's real number solution, the discriminant needs to be greater than or equal to zero
	// B^2 - 4AC
	auto discriminant = (HALF_B * HALF_B - A * C);
	if (discriminant < 0)
	{
		return -1.0;
	}
	else
	{
		//assume the closest hit point (smallest t), so we use [-sqrt(discriminant)] as the solution
		return (-HALF_B - std::sqrt(discriminant)) / A;
	}
}

color ray_color(const ray& r, const hittable& world, int depth)
{
	if (depth < 0)
		return color(0, 0, 0);

	hit_record record;
	if (world.hit(r, 0.001, RT_infinity, record))
	{
		vec3 target = record.p + record.normal + vec3::random_in_hemisphere(record.normal);
		return 0.5 * ray_color(ray(record.p, target-record.p), world, depth-1);
	}

	vec3 ray_dir = unit_vector(r.direction()); // because r.direction() is not normalized
	auto t = 0.5 * (ray_dir.y() + 1.0); // t <- [0, 1]
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); // white blend with blue graident
}



SYS_PROCESS_PARAM(1001, 0x100000);

static u32 running = 0;

extern "C" {

static void program_exit_callback()
{
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
	gcmSetWaitFlip(context);
	rsxFinish(context,1);
}

static void sysutil_exit_callback(u64 status,u64 param,void *usrdata)
{
	switch(status) {
		case SYSUTIL_EXIT_GAME:
			running = 0;
			break;
		case SYSUTIL_DRAW_BEGIN:
		case SYSUTIL_DRAW_END:
			break;
		default:
			break;
	}
}

void blit_simple(Bitmap *bitmap, u32 dstX, u32 dstY, u32 srcX, u32 srcY, u32 w, u32 h)
{
  rsxSetTransferImage(context, GCM_TRANSFER_LOCAL_TO_LOCAL,
    color_offset[curr_fb], color_pitch, dstX, dstY,
    bitmap->offset, bitmap->width*4, rsxGetFixedUint16((float)srcX),
    rsxGetFixedUint16((float)srcY), w, h, 4);
}

void blit_to(Bitmap *bitmap, u32 dstX, u32 dstY, u32 dstW, u32 dstH, u32 srcX, u32 srcY, u32 srcW, u32 srcH)
{
	gcmTransferScale scale;
	gcmTransferSurface surface;

	scale.conversion = GCM_TRANSFER_CONVERSION_TRUNCATE;
	scale.format = GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8;
	scale.origin = GCM_TRANSFER_ORIGIN_CORNER;
	scale.operation = GCM_TRANSFER_OPERATION_SRCCOPY_AND;
	scale.interp = GCM_TRANSFER_INTERPOLATOR_NEAREST;
	scale.clipX = 0;
	scale.clipY = 0;
	scale.clipW = display_width;
	scale.clipH = display_height;
	scale.outX = dstX;
	scale.outY = dstY;
	scale.outW = dstW;
	scale.outH = dstH;

	scale.ratioX = rsxGetFixedSint32((float)srcW / dstW);
	scale.ratioY = rsxGetFixedSint32((float)srcH / dstH);

	scale.inX = rsxGetFixedUint16(srcX);
	scale.inY = rsxGetFixedUint16(srcY);
	scale.inW = bitmap->width;
	scale.inH = bitmap->height;
	scale.offset = bitmap->offset;
	scale.pitch = sizeof(u32) * bitmap->width;

	surface.format = GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8;
	surface.pitch = color_pitch;
	surface.offset = color_offset[curr_fb];

	rsxSetTransferScaleMode(context, GCM_TRANSFER_LOCAL_TO_LOCAL, GCM_TRANSFER_SURFACE);
	rsxSetTransferScaleSurface(context, &scale, &surface);
}

void rsxClearScreenSetBlendState(u32 clear_color)
{
	rsxSetClearColor(context, clear_color);
    rsxSetClearDepthStencil(context, 0xffff);
    rsxClearSurface(context,GCM_CLEAR_R |
                                    GCM_CLEAR_G |
                                    GCM_CLEAR_B |
                                    GCM_CLEAR_A |
                                    GCM_CLEAR_S |
                                    GCM_CLEAR_Z);

    /* Enable blending (for rsxSetTransferScaleSurface) */
    rsxSetBlendFunc(context, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA);
    rsxSetBlendEquation(context, GCM_FUNC_ADD, GCM_FUNC_ADD);
    rsxSetBlendEnable(context, GCM_TRUE);

}

// SPU TEST
#include <sys/spu.h>

#include "spu_bin.h"

#include "spu_shared.h"

#define ptr2ea(x) ((u64)(void *)(x))

// Hijack printf
#define printf debug_printf

#define SPU_COUNT 6
#define SPU_ALIGN 16

// multisample
#define SAMPLES_PER_PIXEL (100)
// ray bounce
#define MAX_BOUNCE_DEPTH (50)


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

struct spu_device_t
{
	spu_shared_t* spu = nullptr;

	spu_device_t(uint32_t spu_index)
	{
		spu = (spu_shared_t*)memalign(SPU_ALIGN, sizeof(spu_shared_t));
		spu->rank = spu_index;
		spu->count = SPU_COUNT;
		spu->sync = SYNC_INVALID;
		// uninitialised job data
		spu->i_data_ea = 0;
		spu->i_data_sz = 0;
		spu->o_data_ea = 0;
		spu->o_data_sz = 0;
	}

	void update_job(spu_job_t* spu_job)
	{
		world_data_t* input = spu_job->input;
		pixel_data_t* output = spu_job->output;

		spu->i_data_ea = ptr2ea(input);
		spu->i_data_sz = sizeof(world_data_t);
		spu->o_data_ea = ptr2ea(output);
		spu->o_data_sz = spu_job->output_size;
	}

	uint32_t pre_kickoff(sysSpuImage* image, sysSpuThreadAttribute* attr, uint32_t group_id)
	{
		uint32_t spuRet = 0;
		sysSpuThreadArgument arg;
		arg.arg0 = ptr2ea(spu);

		debug_printf("Creating SPU thread... ");
		spuRet = sysSpuThreadInitialize(&spu->id, group_id, spu->rank, image, attr, &arg);
		debug_printf("%08x\n", spuRet);
		debug_printf("thread id = %d\n", spu->id);

		debug_printf("Configuring SPU... ");
		spuRet = sysSpuThreadSetConfiguration(spu->id, SPU_SIGNAL1_OVERWRITE|SPU_SIGNAL2_OVERWRITE);
		debug_printf("%08x\n", spuRet);

		return spuRet;
	}

	uint32_t signal_start()
	{
		spu->sync = SYNC_START;
		return sysSpuThreadWriteSignal(spu->id, 0, 1);
	}

	uint32_t signal_quit()
	{
		spu->sync = SYNC_QUIT;
		return sysSpuThreadWriteSignal(spu->id, 0, 1);
	}

	uint32_t check_sync() const
	{
		return spu->sync;
	}

	~spu_device_t()
	{
		free(spu);
	}
};


static std::vector<std::shared_ptr<spu_job_t>> s_spu_jobs;
static std::vector<std::shared_ptr<spu_device_t>> s_spu_devices;

bool check_pad_input()
{
	padInfo padinfo;
	padData paddata;


	sysUtilCheckCallback();
	ioPadGetInfo(&padinfo);
	for(int p=0; p < MAX_PADS; p++)
	{
		if(padinfo.status[p])
		{
			ioPadGetData(p, &paddata);
			if(paddata.BTN_CROSS)
				return true;
		}
	}

	return false;
}

int main()
{
	debug_init();

	// frame buffer
	void *host_addr = memalign(CB_SIZE, HOST_SIZE);
	init_screen(host_addr, HOST_SIZE);

	// handle program exit
	atexit(program_exit_callback);
	sysUtilRegisterCallback(0,sysutil_exit_callback,NULL);

	// input
	ioPadInit(7);

	struct timeval time_now{};
    gettimeofday(&time_now, nullptr);
    int32_t start_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;

	// camera
	camera cam;

	// world
	std::vector<std::shared_ptr<sphere>> sphere_world;
	sphere_world.push_back(make_shared<sphere>(point3(0, 0, -1), 0.5));
	sphere_world.push_back(make_shared<sphere>(point3(1, 0, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(-1, 0, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(0, 1, -1), 0.5));
	sphere_world.push_back(make_shared<sphere>(point3(1, 1, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(-1, 1, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(0, -100.5, 0), 100.0));

	// needs to be big enough to hold all data
	const uint32_t blob_size = 1024;
	uint8_t* obj_blob = (uint8_t*)memalign(SPU_ALIGN, blob_size); 
	uint8_t element_size = 0;
	uint32_t actual_blob_size = 0;

	// serialise two spheres
	for(const auto& sphere : sphere_world)
	{
		sphere->serialise(obj_blob + actual_blob_size, &element_size);
		actual_blob_size += element_size;	
	}

	assert(actual_blob_size <= blob_size);
	debug_printf("Sphere blob addr: %08x size: %d actual_size: %d count: %d\n", obj_blob, blob_size, actual_blob_size, actual_blob_size / element_size);



	// img
	const int img_width = 400;
	const int img_height = (int)(img_width / cam.aspect_ratio);

	debug_printf("Image %dx%d\n", img_width, img_height);

	// ps3 bitmap
	Bitmap bitmap;
	bitmapInit(&bitmap, img_width, img_height);
	setRenderTarget(curr_fb);

	// clear screen
	const u32 CLEAR_COLOR = 0x20FF30;
	rsxClearScreenSetBlendState(CLEAR_COLOR);
	flip();


	/////////////////////////////
	// FOR SPU DEVICES
	bool force_terminate_spu = false;
	uint32_t spuRet = 0;
	sysSpuImage image;
	debug_printf("Initializing %d SPUs... ", SPU_COUNT);
	spuRet = sysSpuInitialize(SPU_COUNT, 0);
	debug_printf("%08x\n", spuRet);

	debug_printf("Loading ELF image... ");
	spuRet = sysSpuImageImport(&image, spu_bin, 0);
	debug_printf("%08x\n", spuRet);

	uint32_t group_id = 0;
	sysSpuThreadAttribute attr = { "mythread", 8+1, SPU_THREAD_ATTR_NONE };
	sysSpuThreadGroupAttribute grpattr = { 7+1, "mygroup", 0, {0} };

	debug_printf("Creating thread group... ");
	spuRet = sysSpuThreadGroupCreate(&group_id, SPU_COUNT, 100, &grpattr);
	debug_printf("%08x\n", spuRet);
	debug_printf("group id = %d\n", group_id);

	for(int i = 0; i < SPU_COUNT; ++i)
	{
		auto spu_device = std::make_shared<spu_device_t>(i);
		spu_device->pre_kickoff(&image, &attr, group_id);
		s_spu_devices.push_back(spu_device);
	}
	
	// This should start all SPU threads already
	debug_printf("Starting SPU thread group... ");
	spuRet = sysSpuThreadGroupStart(group_id);
	debug_printf("%08x\n", spuRet);

	int32_t leftover_scanline = img_height;

	while(leftover_scanline > 0)
	{
		const int32_t min_scanlines_per_job = 1;
		int32_t scanlines_per_job = 10; // make sure we don't exceed SPU's local storage limit
		scanlines_per_job = std::max(min_scanlines_per_job, scanlines_per_job);

		bool job_enough = false;
		int32_t start_scaneline = img_height - leftover_scanline;
		for(int i = 0; i < SPU_COUNT; ++i)
		{
			int32_t start_y = start_scaneline + i * scanlines_per_job;
			int32_t end_y = start_y + scanlines_per_job;

			if (end_y > img_height)
			{
				end_y = img_height;
				job_enough = true;
			}

			s_spu_jobs.push_back(
				std::make_shared<spu_job_t>(img_width, img_height, start_y, end_y, i, obj_blob, actual_blob_size));

			if (job_enough)
				break;
		}

		leftover_scanline = img_height - s_spu_jobs[s_spu_jobs.size() - 1]->input->end_y;

		for(const auto &job : s_spu_jobs)
		{
			job->print_spu_job();
		}

		/////////////////////////////
		// FOR SPU JOB -> SPU DEVICE
		for(auto &job : s_spu_jobs)
		{
			auto spu_device = s_spu_devices[job->spu_index];
			spu_device->update_job(job.get());

			// spu should start at the beginning of the loop by now
			debug_printf("Sending START signal... %d\n", job->spu_index); 
			spu_device->signal_start(); 
		}

		running = 1;
		
		debug_printf("%d scanelines are left\n", leftover_scanline);
		debug_printf("Waiting for SPUs to return...\n");
		bool spu_processed[SPU_COUNT];
		for(size_t i = 0; i < SPU_COUNT; ++i)
		{
			spu_processed[i] = false;
		}

		size_t spu_processed_count = 0;

		while(true)
		{
			for (size_t n = 0; n < s_spu_jobs.size(); n++)
			{
				if (check_pad_input() || !running)
				{
					force_terminate_spu = true;
					goto done;
				}

				if (!spu_processed[n] && s_spu_devices[n]->check_sync() == SYNC_FINISHED)
				{
					spu_processed[n] = true;
					// update bitmap
					spu_processed_count++;

					debug_printf("\nDraw SPU %d result\n", n);
					world_data_t* world = s_spu_jobs[n]->input;
					pixel_data_t* output_pixels = s_spu_jobs[n]->output;
					for(int j = world->start_y; j < world->end_y; ++j)
					{
						for(int i = world->start_x; i < world->end_x; ++i)
						{
							auto spu_y = j - world->start_y;

							//debug_printf("\rWrite pixel at col %d from spu_y: %d", i, spu_y);
							pixel_data_t* pixel = &output_pixels[ spu_y * img_width + i];
							write_pixel_bitmap(&bitmap, i, (img_height - (j+1)), pixel);
						}
					}

					// update screen for every spu job result
					rsxClearScreenSetBlendState(CLEAR_COLOR);
					blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
					flip();
				}

				usleep(32000);

				// SPU work finished, print out result
	#if 0
				pixel_data_t* output_pixels = spu_pixels[i];
				int32_t output_pixels_count = spu_pixels_size[i] / sizeof(pixel_data_t);

				for(int n = 0; n < output_pixels_count; ++n)
				{
					debug_printf("SPU %d Pixel %d: (%.3f, %.3f, %.3f, %.3f)\n", i, n,
						output_pixels[n].rgba[0],
						output_pixels[n].rgba[1],
						output_pixels[n].rgba[2],
						output_pixels[n].rgba[3]);
				}
	#endif
			}

			if (spu_processed_count == s_spu_jobs.size())
				break;
		}

		////////////////////////////
		// SPU JOBS FINALIZE
		s_spu_jobs.clear();
	}




	// Actually this code starts from the top of the image, as the coordinate of the image is Y pointing upwards,
	// then j = img_height-1 means the top row of the image
	/*
	for(int j = img_height-1; j >= 0; --j)
	{
		debug_printf("\rScanline remaining: %d ", j);
		for(int i = 0; i < img_width; ++i)
		{
			color pixel_color(0, 0, 0);
			for (int s = 0; s < sample_per_pixel; ++s)
			{
				if(check_pad_input() || !running)
					goto done;

				double u = ((double)i + random_double()) / (img_width - 1);
				double v = ((double)j + random_double()) / (img_height - 1);

				pixel_color += ray_color(cam.get_ray(u, v), world, max_depth);
			}

			// Bitmap using origin on top-left, with Y axis pointing down, so have to do the conversion here
			write_color_bitmap(&bitmap, i, (img_height - (j+1)), pixel_color, sample_per_pixel);
		}

		if (j % 10 == 0)
		{
			rsxClearScreenSetBlendState(CLEAR_COLOR);
			blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
			flip();
		}
	}
	*/

done:


	////////////////////////////
	// FOR SPU JOB SHUTDOWN
	debug_printf("Signaling all SPUs to quit... ");
	for(auto& device : s_spu_devices)
	{
		device->signal_quit();
	}

#if STOP_SPU_BY_TERMINATION
	debug_printf("\nTerminating SPU thread group %d...", group_id);
	spuRet = sysSpuThreadGroupTerminate(group_id, 0);
	debug_printf("%08x\n", spuRet);
#else
	if (force_terminate_spu)
	{
		debug_printf("\nTerminating SPU thread group %d...", group_id);
		spuRet = sysSpuThreadGroupTerminate(group_id, 0);
		debug_printf("%08x\n", spuRet);
	}
	else 
	{
		u32 cause, status;
		debug_printf("\nJoining SPU thread group %d... ", group_id);
		spuRet = sysSpuThreadGroupJoin(group_id, &cause, &status);
		debug_printf("%08x\n cause=%d status=%d\n", spuRet, cause, status);
	}
#endif

	////////////////////////////
	// FOR SPU DEVICE SHUTDOWN
	s_spu_devices.clear();

	debug_printf("Closing SPU image... ");
	spuRet = sysSpuImageClose(&image);
	debug_printf("%08x\n", spuRet);

	// free world
	free(obj_blob);

	rsxClearScreenSetBlendState(CLEAR_COLOR);
	blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
	flip();

	gettimeofday(&time_now, nullptr);
	int32_t end_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;

	debug_printf("\nDone.\n");
	debug_printf("Time used: %dms\n", end_millisecond - start_millisecond);
	debug_printf("Ray per second: %.2f\n", float(img_width * img_height * SAMPLES_PER_PIXEL) / (end_millisecond - start_millisecond));

	while(!check_pad_input())
		usleep(32000);
	
	debug_printf("Exiting...\n");
	bitmapDestroy(&bitmap);
	debug_return_ps3loadx();
	return 0;
}

} // extern "C"