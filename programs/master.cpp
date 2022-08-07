#include "bitmap.h"
#include "rsxutil.h"
#include "blit.h"
#include <sys/time.h>
#include <sys/mutex.h>
#include <sys/cond.h>
#include <io/pad.h>
#include <assert.h>
#include <malloc.h>
#include <map>
#include <stdint.h>
#include <unistd.h>
#include <sysutil/sysutil.h>

#include "debug.h"
#include "master.h"
#include "network.h"
#include "node_job.h"
#include "spu_shared.h"
#include "color.h"
#include "camera.h"

extern "C" bool check_pad_input();
extern u32 g_running;
extern std::map<uint32_t, node_result_t> g_node_results;
extern node_sync_t g_node_sync;

static const u32 CLEAR_COLOR = 0x20FF30;

static void master_work(Bitmap* bitmap)
{
	const u64 WAIT_TIMEOUT = 1000000;
	while(g_running)
	{
		sysUtilCheckCallback();

		// no more jobs needed and no job is waiting for a result, all the work has been collected
		if (current_node_job_count() == 0 && !require_more_node_jobs())
			break;

		if (check_pad_input() || !g_running)
			break;

		// wait for conditional variable
		//debug_printf("Waiting for condition\n");
        sysMutexLock(g_node_sync.mutex, 0);
        if (sysCondWait(g_node_sync.cond_var, WAIT_TIMEOUT) == 0)
        {

	        debug_printf("Condition signaled\n");

	        sysMutexLock(g_node_sync.util_mutex, 0);

	        // get a done node result from the node container
	        for(auto& pair: g_node_results)
	        {
	        	uint32_t job_id = pair.first;
	        	node_result_t& node_result = pair.second;

	        	// find job details
	        	node_job_t node_job;
	        	if (get_node_job(job_id, &node_job))
	        	{
	        		debug_printf("Processing result of job %d\n", job_id);
	        		// update bitmap
	        		auto img_width = node_job.img_width;
	        		auto img_height = node_job.img_height;
	        		for(int j = node_job.start_y; j < node_job.end_y; ++j)
					{
						auto diff_y = j - node_job.start_y;
						for(int i = node_job.start_x; i < node_job.end_x; ++i)
						{
							pixel_data_t* pixel = &node_result.pixel_data[ diff_y * img_width + i];
							write_pixel_bitmap(bitmap, i, (img_height - (j+1)), pixel);
						}
					}


	        		// dismiss the job
	        		dismiss_node_job(job_id);

	        		debug_printf("Dismissed result of job %d\n", job_id);
	        	}
	        	else
	        	{
	        		debug_printf("Cannot find job: %d. Ignore the job result.\n", job_id);
	        	}
	        }

	        g_node_results.clear();
	        sysMutexUnlock(g_node_sync.util_mutex);

	        debug_printf("Update screen\n");
	        // update screen
	        rsxClearScreenSetBlendState(CLEAR_COLOR);
			blit_to(bitmap, 0, 0, display_width, display_height, 0, 0, bitmap->width, bitmap->height);
			flip();
		}
		else
		{
			debug_printf(".");
		}

		sysMutexUnlock(g_node_sync.mutex);

		usleep(1000);
	}
}

void master_main()
{
	// camera
    camera cam;

    // img
	const int img_width = 400;
	const int img_height = (int)(img_width / cam.aspect_ratio);

	debug_printf("Image %dx%d\n", img_width, img_height);

	// init node jobs before the server runs!
	init_node_jobs(img_width, img_height);


	if (!start_listening_server())
		return;

	// sync obj
	node_sync_init(&g_node_sync);

	// gfx init
	// frame buffer
	void *host_addr = memalign(CB_SIZE, HOST_SIZE);
	init_screen(host_addr, HOST_SIZE);


	// input
	ioPadInit(7);

	struct timeval time_now{};
    gettimeofday(&time_now, nullptr);
    int32_t start_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;

	// ps3 bitmap
	Bitmap bitmap;
	bitmapInit(&bitmap, img_width, img_height);
	setRenderTarget(curr_fb);

	// clear screen
	const u32 CLEAR_COLOR = 0x20FF30;
	rsxClearScreenSetBlendState(CLEAR_COLOR);
	flip();

	g_running = 1;

	// wait for network results and update accordingly
	master_work(&bitmap);

	// gfx shutdown
	rsxClearScreenSetBlendState(CLEAR_COLOR);
	blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
	flip();

	gettimeofday(&time_now, nullptr);
	int32_t end_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;

	debug_printf("\nDone.\n");
	debug_printf("Time used: %dms\n", end_millisecond - start_millisecond);
	debug_printf("Ray per second: %.2f\n", float(img_width * img_height * SAMPLES_PER_PIXEL) / (end_millisecond - start_millisecond));
	
	bitmapDestroy(&bitmap);

	if (!shutdown_listening_server())
		return;


	node_sync_destroy(&g_node_sync);

	shutdown_node_jobs();

	debug_printf("Exiting...\n");
}
