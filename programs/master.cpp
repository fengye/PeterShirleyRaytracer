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
#include "tile.h"

#define WAIT_INPUT_WHEN_FINISH 1

extern "C" bool check_pad_input();
extern u32 g_running;
bool g_master_quit = false;
extern std::map<uint32_t, node_result_t> g_node_results;
extern node_sync_t g_node_sync;

extern u32 display_width; // for blit interpolation
extern u32 display_height;

static const u32 CLEAR_COLOR = 0x20FF30;

static void initTileCursorBitmap(Bitmap* tileCursorBitmap)
{
	bitmapInit(tileCursorBitmap, TILE_WIDTH, TILE_HEIGHT);
	bitmapUpdateSubregion(tileCursorBitmap, 0, 0, TILE_WIDTH-1, TILE_HEIGHT-1, 0, 0, 0, 0);

	for(int x = 0; x < 3; ++x)
	{
		bitmapUpdateRGBA(tileCursorBitmap, x, 0, 255, 255, 255, 255);
		bitmapUpdateRGBA(tileCursorBitmap, x, TILE_HEIGHT-1, 255, 255, 255, 255);
	}

	for(int x = TILE_WIDTH-3; x < TILE_WIDTH; ++x)
	{
		bitmapUpdateRGBA(tileCursorBitmap, x, 0, 255, 255, 255, 255);
		bitmapUpdateRGBA(tileCursorBitmap, x, TILE_HEIGHT-1, 255, 255, 255, 255);
	}

	for(int y = 0; y < 3; ++y)
	{
		bitmapUpdateRGBA(tileCursorBitmap, 0, y, 255, 255, 255, 255);
		bitmapUpdateRGBA(tileCursorBitmap, TILE_WIDTH-1, y, 255, 255, 255, 255);
	}

	for(int y = TILE_HEIGHT-3; y < TILE_HEIGHT; ++y)
	{
		bitmapUpdateRGBA(tileCursorBitmap, 0, y, 255, 255, 255, 255);
		bitmapUpdateRGBA(tileCursorBitmap, TILE_WIDTH-1, y, 255, 255, 255, 255);
	}
}

static void destroyTileCursorBitmap(Bitmap* tileCursorBitmap)
{
	bitmapDestroy(tileCursorBitmap);
}

static void master_work(Bitmap* bitmap)
{
	const u64 WAIT_TIMEOUT = 1000000;

	Bitmap tileCursorBitmap;
	initTileCursorBitmap(&tileCursorBitmap);

	while(g_running)
	{
		struct timeval time_now{};
	    gettimeofday(&time_now, nullptr);
	    int32_t start_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;



		while(is_rendering_curr_frame())
		{
			sysUtilCheckCallback();

			// no more jobs needed and no job is waiting for a result, all the work has been collected
			if (current_node_job_count() == 0 && !require_more_node_jobs())
			{
				complete_curr_frame();
				break;
			}

			if (check_pad_input() || !g_running)
			{
				debug_printf("Detected Master Quit!\n");
				g_master_quit = true;
				break;
			}

			// wait for conditional variable
			//debug_printf("Waiting for condition\n");
	        //sysMutexLock(g_node_sync.mutex, 0);
	        //if (sysCondWait(g_node_sync.cond_var, WAIT_TIMEOUT) == 0)
	        if (node_sync_wait(&g_node_sync, WAIT_TIMEOUT))
	        {

		        debug_printf("Condition signaled\n");

		        for(auto& pair: g_node_results)
		        {
		        	uint32_t job_id = pair.first;	
		        	debug_printf("Existing result: job(%d)\n", job_id);
		        }

		        // get a done node result from the node container
		        for(auto& pair: g_node_results)
		        {
		        	uint32_t job_id = pair.first;
		        	node_result_t& node_result = pair.second;

		        	// find job details
		        	node_job_t node_job;
		        	if (get_node_job(job_id, &node_job))
		        	{
		        		// precalc some of the screen parameters
			        	const float u_delta = ((float)TILE_WIDTH + 0.5f) / (float)node_job.img_width;
			        	const float v_delta = ((float)TILE_HEIGHT + 0.5f) / (float)node_job.img_height;
			        	const u32 tileScreenWidth = (u32)(u_delta * display_width);
			        	const u32 tileScreenHeight = (u32)(v_delta * display_height);


		        		//debug_printf("Processing result of job %d\n", job_id);

		        		// work out global pixel coordinate of node_result.start_tile_id till end_tile_id
		        		// then update bitmap with correct mapping

		        		// for each tile
		        		for(int i = node_result.start_tile_id; i <= node_result.end_tile_id; ++i)
		        		{
		        			int tile_left_pos, tile_top_pos;
		        			get_tile_coord(i, &tile_left_pos, &tile_top_pos);

		        			//debug_printf("Tile %d coord: (%d, %d)\n", i, tile_left_pos, tile_top_pos);

	        				for(int y = 0; y < TILE_HEIGHT; ++y)
	        				{
	        					for(int x = 0; x < TILE_WIDTH; ++x)
		        				{
		        					pixel_data_t pixel;
		        					// read pixel from node result
		        					//debug_printf("Get local pixel (%d, %d) from tile %d(offset %d)\n", x, y, i,  i - node_result.start_tile_id);
		        					get_local_pixel_from_tile_offset(node_result.pixel_data, i - node_result.start_tile_id, x, y, &pixel);
		        					// write pixel back to global bitmap
		        					int global_x = tile_left_pos + x;
		        					int global_y = tile_top_pos + y;
		        					if (global_x >= node_job.img_width ||
		        						global_y >= node_job.img_height)
		        					{
		        						// those pixels aren't valid
		        						//debug_printf("Out of bounds: (%d, %d) ignored.\n", global_x, global_y);
		        						continue;
		        					}
		        					//debug_printf("Write global pixel (%d, %d)\n", global_x, global_y);
		        					write_pixel_bitmap(bitmap, global_x, global_y, &pixel);
		        				}
		        			}

		        			//debug_printf("Update screen for tile %d\n", i);
					        // update screen

		        			float u = ((float)tile_left_pos + 0.5f) / (float)node_job.img_width;
		        			float v = ((float)tile_top_pos + 0.5f) / (float)node_job.img_height;
		        			u32 tileScreenX = (u32)(u * display_width);
		        			u32 tileScreenY = (u32)(v * display_height);

		        			// blit the whole image
		        			blit_to(bitmap, 0, 0, display_width, display_height, 0, 0, bitmap->width, bitmap->height);
		        			// blit cursor 
		        			blit_to(&tileCursorBitmap, tileScreenX, tileScreenY, tileScreenWidth, tileScreenHeight, 0, 0, TILE_WIDTH, TILE_HEIGHT);
							flip();
	        			
		        		}

		        		// dismiss the job
		        		dismiss_node_job(job_id);

		        		//debug_printf("Dismissed result of job %d\n", job_id);
		        	}
		        	else
		        	{
		        		debug_printf("Cannot find job: %d. Ignore the job result.\n", job_id);
		        	}
		        }

		        g_node_results.clear();

		        
			}
			else
			{
				debug_printf_noprefix(".");
			}

			sysMutexUnlock(g_node_sync.mutex);

			usleep(1000);
		}

		if (g_master_quit)
		{
			break;
		}

		gettimeofday(&time_now, nullptr);
		int32_t end_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;
		debug_printf("Rendering frame %d used: %dms\n", get_curr_frame(), end_millisecond - start_millisecond);
		debug_printf("Ray per second: %.2f\n", float(bitmap->width * bitmap->height * SAMPLES_PER_PIXEL) / (end_millisecond - start_millisecond));

		advance_next_frame();
	}

	destroyTileCursorBitmap(&tileCursorBitmap);

	debug_printf("End of master_work()\n");
}

void master_main()
{
	// camera
    camera cam;

    // img
	const int img_width = 400;
	const int img_height = (int)(img_width / cam.aspect_ratio);

	debug_printf("Image %dx%d\n", img_width, img_height);

	g_running = 1;

	// init tiles before init_node_jobs as it depends on how tiles being constructed
	init_tiles(img_width, img_height);
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


	// ps3 bitmap
	Bitmap bitmap;
	bitmapInit(&bitmap, img_width, img_height);
	setRenderTarget(curr_fb);

	// clear screen
	const u32 CLEAR_COLOR = 0x20FF30;
	rsxClearScreenSetBlendState(CLEAR_COLOR);
	flip();

	debug_printf("Waiting for input to kick off nodes...\n");
	while(!check_pad_input())
	{
		sysUtilCheckCallback();
		rsxClearScreenSetBlendState(CLEAR_COLOR);
		flip();
		usleep(2000);
	}

	debug_printf("Kick off nodes.\n");
	kickoff_all_nodes();

	struct timeval time_now{};
    gettimeofday(&time_now, nullptr);
    int32_t start_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;

	// wait for network results and update accordingly
	master_work(&bitmap);

	// gfx shutdown
	blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
	flip();

	gettimeofday(&time_now, nullptr);
	int32_t end_millisecond = (time_now.tv_sec) * 1000 + (time_now.tv_usec) / 1000;

	debug_printf("\nDone.\n");
	debug_printf("Total Time used: %dms\n", end_millisecond - start_millisecond);
	
	
	if (!shutdown_listening_server())
		return;

	node_sync_destroy(&g_node_sync);

	shutdown_node_jobs();

#if WAIT_INPUT_WHEN_FINISH
	debug_printf("All processing done.\n");
	while(!check_pad_input())
	{
		sysUtilCheckCallback();
		blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
		flip();
		usleep(32000);
	}

	debug_printf("Exited program after button press.\n");
#else
	debug_printf("Exited.\n");
#endif

	bitmapDestroy(&bitmap);
}
