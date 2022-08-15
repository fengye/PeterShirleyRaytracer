#include "config.h"
#include "network.h"
#include "spu_device.h"
#include "ppu_device.h"
#include "slave.h"

#include <unistd.h>
#include <vector>
#include <memory>
#include <malloc.h>
#include <string.h>
// PS3 specific
#include <sysutil/sysutil.h>
#include <sys/process.h>
// ~PS3 specific
// SPU specific
#include <sys/spu.h>
#include "spu_bin.h"
#include "spu_job.h"
#include "spu_device.h"
#include "spu_shared.h"
// ~SPU specific
#include "ppu_device.h"
#include "ppu_job.h"


#define STOP_SPU_BY_TERMINATION 0

extern "C" bool check_pad_input();
extern u32 g_running;
extern bool g_is_slave_on_master;

static std::vector<std::shared_ptr<spu_device_t>> s_spu_devices;
static std::vector<std::shared_ptr<ppu_device_t>> s_ppu_devices;
static uint32_t s_spu_group_id = 0;
static std::vector<std::shared_ptr<job_t>> s_all_jobs;
static bool s_force_terminate_spu = false;
static sysSpuImage s_spu_image;

static void slave_init_spus(int spu_count)
{
	/////////////////////////////
	// FOR SPU DEVICES
	s_force_terminate_spu = false;
	uint32_t spuRet = 0;
	
	debug_printf("Initializing %d SPUs... ", spu_count);
	spuRet = sysSpuInitialize(spu_count, 0);
	debug_printf("%08x\n", spuRet);

	debug_printf("Loading ELF image... ");
	spuRet = sysSpuImageImport(&s_spu_image, spu_bin, 0);
	debug_printf("%08x\n", spuRet);

	sysSpuThreadAttribute attr = { "mythread", 8+1, SPU_THREAD_ATTR_NONE };
	sysSpuThreadGroupAttribute grpattr = { 7+1, "mygroup", 0, {0} };

	debug_printf("Creating thread group... ");
	spuRet = sysSpuThreadGroupCreate(&s_spu_group_id, spu_count, 100, &grpattr);
	debug_printf("%08x\n", spuRet);
	debug_printf("group id = %d\n", s_spu_group_id);

	for(int i = 0; i < spu_count; ++i)
	{
		auto spu_device = std::make_shared<spu_device_t>(i);
		spu_device->pre_kickoff(&s_spu_image, &attr, s_spu_group_id);
		s_spu_devices.push_back(spu_device);
	}
	
	// This should start all SPU threads already
	debug_printf("Starting SPU thread group... ");
	spuRet = sysSpuThreadGroupStart(s_spu_group_id);
	debug_printf("%08x\n", spuRet);
}

static void slave_shutdown_spus()
{
	uint32_t spuRet = 0;

	////////////////////////////
	// FOR SPU JOB SHUTDOWN
	debug_printf("Signaling all SPUs to quit...\n");
	for(auto& device : s_spu_devices)
	{
		device->signal_quit();
	}

#if STOP_SPU_BY_TERMINATION
	debug_printf("\nTerminating SPU thread group %d...", s_spu_group_id);
	spuRet = sysSpuThreadGroupTerminate(s_spu_group_id, 0);
	debug_printf("%08x\n", spuRet);
#else
	if (s_force_terminate_spu)
	{
		debug_printf("\nTerminating SPU thread group %d...", s_spu_group_id);
		spuRet = sysSpuThreadGroupTerminate(s_spu_group_id, 0);
		debug_printf("%08x\n", spuRet);
	}
	else 
	{
		u32 cause, status;
		debug_printf("\nJoining SPU thread group %d... ", s_spu_group_id);
		spuRet = sysSpuThreadGroupJoin(s_spu_group_id, &cause, &status);
		debug_printf("%08x\n cause=%d status=%d\n", spuRet, cause, status);
	}
#endif

	////////////////////////////
	// FOR SPU DEVICE SHUTDOWN
	s_spu_devices.clear();

	debug_printf("Closing SPU image... ");
	spuRet = sysSpuImageClose(&s_spu_image);
	debug_printf("%08x\n", spuRet);
}

static void slave_init_ppus(int ppu_count)
{
	/////////////////////////////
	// FOR PPU DEVICES
	for(int i = 0; i < ppu_count; ++i)
	{
		auto ppu_device = std::make_shared<ppu_device_t>(i);
		ppu_device->pre_kickoff();
		s_ppu_devices.push_back(ppu_device);
	}
}

static void slave_shutdown_ppus()
{
	// FOR PPU JOB SHUTDOWN
	debug_printf("Signaling all PPUs to quit...\n");
	for(auto& device : s_ppu_devices)
	{
		device->signal_quit();
	}

	s_ppu_devices.clear();
}

static void slave_work(void* obj_blob, size_t actual_blob_size, int img_width, int img_height, int global_start_y, int global_end_y, uint8_t** out_pixel_data, size_t* out_pixel_data_size)
{
	int32_t leftover_scanline = global_end_y - global_start_y;
	size_t pixel_data_size = img_width * leftover_scanline * sizeof(pixel_data_t);
	uint8_t *pixel_data = (uint8_t*)malloc(pixel_data_size);

	bool quit_work = false;
	bool allowCheckInput = !g_is_slave_on_master;

	while(leftover_scanline > 0 && !quit_work)
	{
		const int32_t min_scanlines_per_job = 1;
		int32_t scanlines_per_job = SPU_CAP; //10; // make sure we don't exceed SPU's local storage limit
		scanlines_per_job = std::max(min_scanlines_per_job, scanlines_per_job);

		bool job_enough = false;
		int32_t start_scaneline = global_end_y - leftover_scanline;
		for(int i = 0; i < SPU_COUNT; ++i)
		{
			int32_t start_y = start_scaneline;
			int32_t end_y = start_y + scanlines_per_job;
			start_scaneline += scanlines_per_job;

			if (end_y > global_end_y)
			{
				end_y = global_end_y;
				start_scaneline = global_end_y;
				job_enough = true;
			}

			s_all_jobs.push_back(
				std::make_shared<spu_job_t>(img_width, img_height, start_y, end_y, i, obj_blob, actual_blob_size));

			if (job_enough)
				break;
		}

		if (!job_enough)
		{
			scanlines_per_job = PPU_CAP;
			for(int i = 0; i < PPU_COUNT; ++i)
			{
				int32_t start_y = start_scaneline;
				int32_t end_y = start_y + scanlines_per_job;
				start_scaneline += scanlines_per_job;

				if (end_y > global_end_y)
				{
					end_y = global_end_y;
					start_scaneline = global_end_y;
					job_enough = true;
				}

				s_all_jobs.push_back(
					std::make_shared<ppu_job_t>(img_width, img_height, start_y, end_y, i, obj_blob, actual_blob_size));

				if (job_enough)
					break;
			}
		}

		leftover_scanline = global_end_y - s_all_jobs[s_all_jobs.size() - 1]->get_input()->end_y;

		for(const auto &job : s_all_jobs)
		{
			job->print_job_details();
		}

		/////////////////////////////
		// SPU/PPU JOB -> SPU/PPU DEVICE
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

		
		
		debug_printf("Local workload: %d scanelines are left\n", leftover_scanline);
		debug_printf("Waiting for SPUs and PPUs to return...\n");
		bool job_processed[SPU_PPU_COUNT];
		for(size_t i = 0; i < SPU_PPU_COUNT; ++i)
		{
			job_processed[i] = false;
		}

		size_t job_processed_count = 0;

		while(!quit_work)
		{
			for (size_t n = 0; n < s_all_jobs.size(); n++)
			{
				sysUtilCheckCallback();

				if ((allowCheckInput && check_pad_input()) || !g_running )
				{
					debug_printf("Detected slave button press or system exit. Force quit!\n");
					s_force_terminate_spu = true;
					quit_work = true;
					break;
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

							debug_printf("Copy SPU %d result\n", n);
							world_data_t* input = s_all_jobs[n]->get_input();
							pixel_data_t* output_pixels = s_all_jobs[n]->get_output();

							size_t job_data_size = (input->end_x - input->start_x) * (input->end_y - input->start_y) * sizeof(pixel_data_t);
							int y_offset = input->start_y - global_start_y;
							memcpy(pixel_data + y_offset * img_width * sizeof(pixel_data_t), output_pixels, job_data_size);
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

							debug_printf("Copy PPU %d result\n", n - SPU_COUNT);
							world_data_t* input = s_all_jobs[n]->get_input();
							pixel_data_t* output_pixels = s_all_jobs[n]->get_output();

							size_t job_data_size = (input->end_x - input->start_x) * (input->end_y - input->start_y) * sizeof(pixel_data_t);
							int y_offset = input->start_y - global_start_y;
							memcpy(pixel_data + y_offset * img_width * sizeof(pixel_data_t), output_pixels, job_data_size);
						}
					}
				}

				usleep(2000);
			}

			if (job_processed_count == s_all_jobs.size())
				break;
		}

		////////////////////////////
		// SPU/PPU JOBS FINALIZE
		s_all_jobs.clear();
	}

	// output
	*out_pixel_data = pixel_data;
	*out_pixel_data_size = pixel_data_size;
}

extern char g_self_addr[256];

// This process should directly corresponding to node_serv_func() in network module
void slave_main(const char* master_addr)
{
	int conn_fd;
	g_running = 1;

	debug_printf("Connecting to master: %s from %s... ", master_addr, g_self_addr);
	if (connect_master(master_addr, &conn_fd))
	{
		debug_printf("Done!\n");

		sysUtilCheckCallback();

		const int ppu_count = PPU_COUNT;
		const int spu_count = SPU_COUNT;

		debug_printf("Slave initializing %d SPUs\n", spu_count);
		slave_init_spus(spu_count);
		debug_printf("Slave initializing %d PPUs\n", ppu_count);
		slave_init_ppus(ppu_count);

		while(g_running)
		{
			bool is_shutdown;
			debug_printf("Receiving shutdown/nop request from %d ... \n", conn_fd);
			if (!recv_shutdown_request(conn_fd, &is_shutdown))
			{
				debug_printf("Error!\n");
				break;
			}
			else
			{
				debug_printf("Receiving shutdown/nop from %d request done.\n", conn_fd);
			}

			if (is_shutdown)
			{
				debug_printf("Received SHUTDOWN request. Quitting.\n");
				break;
			}

			// send offer
			debug_printf("Sending offer (%d, %d) ... to %d\n", ppu_count, spu_count, conn_fd);
			if (!send_processor_offer(conn_fd, ppu_count, spu_count))
			{
				debug_printf("Error!\n");
				break;
			}
			else{
				debug_printf("Sending offer (%d, %d) to %d done.\n", ppu_count, spu_count, conn_fd);
			}

			int32_t job_count;
			debug_printf("Receiving job count from %d ...\n", conn_fd);
			if (!recv_job_count(conn_fd, &job_count))
			{
				debug_printf("Error!\n");
				break;
			}

			if (job_count > 0)
			{
				debug_printf("Job count valid from %d\n", conn_fd);

				// receive job request
				uint32_t job_id;
				uint8_t* world_data;
				size_t world_data_size;
				int img_width, img_height;
				int start_y, end_y;
				debug_printf("Receiving job request from %d ... \n", conn_fd);
				if (!recv_job_request(conn_fd, &job_id, &world_data, &world_data_size, &img_width, &img_height, &start_y, &end_y))
				{
					debug_printf("Error!\n");
					break;
				}
				else
				{
					debug_printf("Receiving job request from %d done.\n", conn_fd);
				}

				// do the work
				uint8_t* pixel_data;
				size_t pixel_data_size;
				debug_printf("Doing raytrace work on %s... \n", g_self_addr);
				slave_work(world_data, world_data_size, img_width, img_height, start_y, end_y, &pixel_data, &pixel_data_size);
				// caller to free the world data
				free(world_data);
				debug_printf("Raytrace work on %s done.\n", g_self_addr);

				// send the result back to master
				debug_printf("Sending job(%d) result %p(%lld) to %d... ", job_id, pixel_data, pixel_data_size, conn_fd);
				if (!send_job_result(conn_fd, job_id, pixel_data, pixel_data_size))
				{
					debug_printf("Error!\n");
					free(pixel_data);
					break;
				}
				else
				{
					debug_printf("Sending job(%d) result %p(%lld) to %d done!\n", job_id, pixel_data, pixel_data_size, conn_fd);
					free(pixel_data);
					debug_printf("Freed pixel data\n");
				}
			}
			else
			{
				debug_printf("No job count for this frame from %d\n", conn_fd);
			}


			debug_printf("Restarting slave routine...\n");
		}

		debug_printf("Slave shutting down %d SPUs\n", spu_count);
		slave_shutdown_ppus();
		debug_printf("Slave shutting down %d PPUs\n", ppu_count);
		slave_shutdown_spus();

		debug_printf("Disconnecting from master %s\n", master_addr);
		disconnect_master();
	}
	else
	{
		debug_printf("FAILED!\n");
	}

}

