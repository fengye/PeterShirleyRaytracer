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
#define SLAVE_ON_MASTER 1  // doing work even I'm the master

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

		slave_main((const char*)args, false);
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
			else  // --slave=192.168.0.x <- this is the master ipaddr
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
		slave_main(master_addr, true);
	}

	debug_return_ps3loadx();
	return 0;
}

} // extern "C"