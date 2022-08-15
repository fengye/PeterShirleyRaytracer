#include "debug.h"

#include "raytracer.h"
#include "config.h"
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
#include <cstring>
#include "network.h"
// PS3 specific
#include <io/pad.h>
#include <rsx/rsx.h>
#include <sysutil/sysutil.h>
#include <sys/process.h>
// ~PS3 specific
#include "bitmap.h"
#include "rsxutil.h"
#include "blit.h"
#include <sys/time.h>
#include <assert.h>
// SPU specific
#include <sys/spu.h>
#include "spu_bin.h"
#include "spu_job.h"
#include "spu_device.h"
#include "spu_shared.h"
// ~SPU specific
#include "ppu_device.h"
#include "ppu_job.h"

#include "master.h"
#include "slave.h"

#define WAIT_INPUT_WHEN_FINISH 1
#define STOP_SPU_BY_TERMINATION 0
#define SINGLE_NODE_MODE 0
#define SLAVE_ON_MASTER 1

SYS_PROCESS_PARAM(1001, 0x100000);

u32 g_running = 0;
bool g_master_mode;

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
			g_running = 0;
			break;
		case SYSUTIL_DRAW_BEGIN:
		case SYSUTIL_DRAW_END:
			break;
		default:
			break;
	}
}


// Hijack printf
#define printf debug_printf

#if SINGLE_NODE_MODE
static std::vector<std::shared_ptr<job_t>> s_all_jobs;
static std::vector<std::shared_ptr<spu_device_t>> s_spu_devices;
static std::vector<std::shared_ptr<ppu_device_t>> s_ppu_devices;
#endif


static padData s_lastPadData[MAX_PADS];
bool check_pad_input()
{
	padInfo padinfo;
	padData paddata;

	bool pressed = false;

	ioPadGetInfo(&padinfo);
	for(int p=0; p < MAX_PADS; p++)
	{
		if(padinfo.status[p])
		{
			ioPadGetData(p, &paddata);
			
			if(paddata.BTN_CROSS && !s_lastPadData[p].BTN_CROSS)
				pressed = true;

			s_lastPadData[p] = paddata;
		}

		if (pressed)
			break;
	}

	return pressed;
}

#if SLAVE_ON_MASTER

node_sync_t g_slave_on_master_sync;
bool g_is_slave_on_master = false;

static void slave_on_master_func(void* args)
{
	// wait until the master listening server says yes
	if (node_sync_wait(&g_slave_on_master_sync, 0))
	{

		slave_main((const char*)args);
	}

	sysThreadExit(0);
}
#endif

char g_self_addr[256] = {0};

int main(int argc, char** argv)
{
	debug_init();

	// handle program exit
	atexit(program_exit_callback);
	sysUtilRegisterCallback(0,sysutil_exit_callback,NULL);

	debug_printf("Args count: %d\n", argc);

	const char* master_addr = NULL;
	get_primary_ip(g_self_addr, 256);
	debug_printf("Current node IP address: %s\n", g_self_addr);
	

	g_master_mode = true;
	if (argc < 3)
	{
		master_addr = g_self_addr;
		debug_printf("Master node at: %s\n", master_addr);
		g_master_mode = true;
	}
	else
	{
		for(int i = 2; i < argc; ++i)
		{
			debug_printf("Arg %d: %s\n", i, argv[i]);

			if (strncmp(argv[i], "--master", 8) == 0 ||
				strncmp(argv[i], "-m", 2) == 0)
			{
				// master node
				master_addr = g_self_addr;
				debug_printf("Master node at: %s\n", master_addr);
				g_master_mode = true;
			}
			else 
			if (strncmp(argv[i], "--slave", 7) == 0)
			{
				// slave node
				if (argv[i][7] == '=')
				{
					master_addr = argv[i] + 8;
					debug_printf("Slave node connecting to: %s\n", master_addr);
					g_master_mode = false;
				}
				else
				{
					debug_printf("Missing master IP address. Quit...\n");
					debug_return_ps3loadx();
					return -1;
				}
			}
			else
			if (strncmp(argv[i], "-s", 2) == 0)
			{
				// slave node
				if (argv[i][2] == '=')
				{
					master_addr = argv[i] + 3;
					debug_printf("Slave node connecting to: %s\n", master_addr);
					g_master_mode = false;
				}
				else 
				{
					debug_printf("Missing master IP address. Quit...\n");
					debug_return_ps3loadx();
					return -1;
				}

			}
			else
			{
				// unknown option, quit
				debug_printf("FATAL: unknown parameter: %s. Quit...\n", argv[i]);
				debug_return_ps3loadx();
				return -1;
			}
		}
	}

#if !SINGLE_NODE_MODE
	if (g_master_mode)
	{
		node_sync_init(&g_slave_on_master_sync);
		
#if SLAVE_ON_MASTER
		g_is_slave_on_master = true;
		sys_ppu_thread_t slave_tid = -1;
		char thread_name[] = "slave_on_master_func";
        if (sysThreadCreate(&slave_tid, slave_on_master_func, (void*)master_addr, 1200, 0x4000, THREAD_JOINABLE, thread_name) < 0)
        {
            debug_printf("Error creating master slave hybrid thread. Only master node will be running.\n");
        }

#endif

		master_main();
		node_sync_destroy(&g_slave_on_master_sync);

#if SLAVE_ON_MASTER
		// join slave hybrid
		if (slave_tid >= 0)
		{
			u64 retval;
			sysThreadJoin(slave_tid, &retval);
		}
#endif

	}
	else
	{
		slave_main(master_addr);
	}

#else
	// frame buffer
	void *host_addr = memalign(CB_SIZE, HOST_SIZE);
	init_screen(host_addr, HOST_SIZE);


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
	/*
	sphere_world.push_back(make_shared<sphere>(point3(1, 0, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(-1, 0, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(0, 1, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(0, 1.5, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(1, 1, -1), 0.25));
	sphere_world.push_back(make_shared<sphere>(point3(-1, 1, -1), 0.25));
	*/
	sphere_world.push_back(make_shared<sphere>(point3(0, -100.5, 0), 100.0));

	// needs to be big enough to hold all data
	const uint32_t blob_size = 1024;
	uint8_t* obj_blob = (uint8_t*)memalign(SPU_ALIGN, blob_size); 
	uint8_t element_size = 0;
	uint32_t actual_blob_size = 0;

	// serialise all the spheres
	for(const auto& sphere : sphere_world)
	{
		sphere->serialise(obj_blob + actual_blob_size, &element_size);
		actual_blob_size += element_size;	
	}

	assert(actual_blob_size <= blob_size);
	debug_printf("Sphere blob addr: %lu size: %d actual_size: %d count: %d\n", (uint64_t)obj_blob, blob_size, actual_blob_size, actual_blob_size / element_size);



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


	/////////////////////////////
	// FOR PPU DEVICES
	for(int i = 0; i < PPU_COUNT; ++i)
	{
		auto ppu_device = std::make_shared<ppu_device_t>(i);
		ppu_device->pre_kickoff();
		s_ppu_devices.push_back(ppu_device);
	}

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
			int32_t start_y = start_scaneline;
			int32_t end_y = start_y + scanlines_per_job;
			start_scaneline += scanlines_per_job;

			if (end_y > img_height)
			{
				end_y = img_height;
				start_scaneline = img_height;
				job_enough = true;
			}

			s_all_jobs.push_back(
				std::make_shared<spu_job_t>(img_width, img_height, start_y, end_y, i, obj_blob, actual_blob_size));

			if (job_enough)
				break;
		}

		if (!job_enough)
		{
			for(int i = 0; i < PPU_COUNT; ++i)
			{
				int32_t start_y = start_scaneline;
				int32_t end_y = start_y + scanlines_per_job;
				start_scaneline += scanlines_per_job;

				if (end_y > img_height)
				{
					end_y = img_height;
					start_scaneline = img_height;
					job_enough = true;
				}

				s_all_jobs.push_back(
					std::make_shared<ppu_job_t>(img_width, img_height, start_y, end_y, i, obj_blob, actual_blob_size));

				if (job_enough)
					break;
			}
		}

		leftover_scanline = img_height - s_all_jobs[s_all_jobs.size() - 1]->get_input()->end_y;

		for(const auto &job : s_all_jobs)
		{
			job->print_job_details();
		}

		/////////////////////////////
		// FOR SPU JOB -> SPU DEVICE
		for(auto &job : s_all_jobs)
		{
			auto processor_index = job->get_processor_index();
			if (processor_index < SPU_COUNT)
			{
				// spu device

				auto spu_device = s_spu_devices[processor_index];
				spu_device->update_job(job.get());

				// spu should start at the beginning of the loop by now
				debug_printf("Sending SPU START signal... %d\n", processor_index); 
				spu_device->signal_start(); 

			}
			else
			{
				// ppu device
				auto ppu_device = s_ppu_devices[processor_index - SPU_COUNT];
				ppu_device->update_job(job.get());

				debug_printf("Sending PPU START signal... %d\n", processor_index); 
				ppu_device->signal_start(); 
			}
			
		}

		g_running = 1;
		
		debug_printf("%d scanelines are left\n", leftover_scanline);
		debug_printf("Waiting for SPUs and PPUs to return...\n");
		bool job_processed[SPU_PPU_COUNT];
		for(size_t i = 0; i < SPU_PPU_COUNT; ++i)
		{
			job_processed[i] = false;
		}

		size_t job_processed_count = 0;

		while(true)
		{
			for (size_t n = 0; n < s_all_jobs.size(); n++)
			{
				if (check_pad_input() || !g_running)
				{
					force_terminate_spu = true;
					goto done;
				}

				auto& job = s_all_jobs[n];
				auto processor_index = job->get_processor_index();
				if (!job_processed[processor_index])
				{
					if (processor_index < SPU_COUNT)
					{
						// it's spu job
						if (s_spu_devices[processor_index]->check_sync() == SYNC_FINISHED)
						{
							job_processed[processor_index] = true;
							// update bitmap
							job_processed_count++;

							debug_printf("\nDraw SPU %d result\n", n);
							world_data_t* world = s_all_jobs[n]->get_input();
							pixel_data_t* output_pixels = s_all_jobs[n]->get_output();
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
					}
					else
					{
						// it's ppu job
						if (s_ppu_devices[processor_index - SPU_COUNT]->check_sync() == SYNC_FINISHED)
						{
							job_processed[processor_index] = true;
							// update bitmap
							job_processed_count++;

							debug_printf("\nDraw PPU %d result\n", n);
							world_data_t* world = s_all_jobs[n]->get_input();
							pixel_data_t* output_pixels = s_all_jobs[n]->get_output();
							for(int j = world->start_y; j < world->end_y; ++j)
							{
								for(int i = world->start_x; i < world->end_x; ++i)
								{
									auto spu_y = j - world->start_y;

									pixel_data_t* pixel = &output_pixels[ spu_y * img_width + i];
									write_pixel_bitmap(&bitmap, i, (img_height - (j+1)), pixel);
								}
							}

							// update screen for every spu job result
							rsxClearScreenSetBlendState(CLEAR_COLOR);
							blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
							flip();

						}
					}
				}

				usleep(2000);

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

			if (job_processed_count == s_all_jobs.size())
				break;
		}

		////////////////////////////
		// SPU JOBS FINALIZE
		s_all_jobs.clear();
	}
done:

	// FOR PPU JOB SHUTDOWN
	debug_printf("Signaling all PPUs to quit...\n");
	for(auto& device : s_ppu_devices)
	{
		device->signal_quit();
	}

	s_ppu_devices.clear();


	////////////////////////////
	// FOR SPU JOB SHUTDOWN
	debug_printf("Signaling all SPUs to quit...\n");
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

	////////////////////////////
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

#if WAIT_INPUT_WHEN_FINISH
	while(!check_pad_input())
		sysUtilCheckCallback();
		blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
		flip();
		usleep(32000);
#endif
	
	debug_printf("Exiting...\n");
	bitmapDestroy(&bitmap);
#endif
	debug_return_ps3loadx();
	return 0;
}

} // extern "C"