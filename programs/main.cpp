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
// PS3 specific
#include <io/pad.h>
#include <rsx/rsx.h>
#include <sysutil/sysutil.h>
#include <sys/process.h>
// ~PS3 specific
#include "bitmap.h"
#include "rsxutil.h"

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
#include "spustr.h"

#define ptr2ea(x) ((u64)(void *)(x))

// Hijack printf
#define printf debug_printf

int spu_test()
{
	sysSpuImage image;
	u32 group_id;
	sysSpuThreadAttribute attr = { "mythread", 8+1, SPU_THREAD_ATTR_NONE };
	sysSpuThreadGroupAttribute grpattr = { 7+1, "mygroup", 0, {0} };
	sysSpuThreadArgument arg[6];
	u32 cause, status;
	int i;
	spustr_t *spu = (spustr_t *)memalign(16, 6*sizeof(spustr_t));
	uint32_t *array = (uint32_t *)memalign(16, 24*sizeof(uint32_t));

	printf("Initializing 6 SPUs... ");
	printf("%08x\n", sysSpuInitialize(6, 0));

	printf("Loading ELF image... ");
	printf("%08x\n", sysSpuImageImport(&image, spu_bin, 0));

	printf("Creating thread group... ");
	printf("%08x\n", sysSpuThreadGroupCreate(&group_id, 6, 100, &grpattr));
	printf("group id = %d\n", group_id);

	/* create 6 spu threads */
	for (i = 0; i < 6; i++) {
		spu[i].rank = i;
		spu[i].count = 6;
		spu[i].sync = 0;
		spu[i].array_ea = ptr2ea(array);
		arg[i].arg0 = ptr2ea(&spu[i]);

		printf("Creating SPU thread... ");
		printf("%08x\n", sysSpuThreadInitialize(&spu[i].id, group_id, i, &image, &attr, &arg[i]));
		printf("thread id = %d\n", spu[i].id);

		printf("Configuring SPU... %08x\n",
		sysSpuThreadSetConfiguration(spu[i].id, SPU_SIGNAL1_OVERWRITE|SPU_SIGNAL2_OVERWRITE));
	}

	printf("Starting SPU thread group... ");
	printf("%08x\n", sysSpuThreadGroupStart(group_id));

	printf("Initial array: ");
	for (i = 0; i < 24; i++) {
		array[i] = i+1;
		printf(" %d", array[i]);
	}
	printf("\n");

	/* Send signal notification to waiting spus */
	for (i = 0; i < 6; i++)
		printf("Sending signal... %08x\n",
			sysSpuThreadWriteSignal(spu[i].id, 0, 1));

	printf("Waiting for SPUs to return...\n");
	for (i = 0; i < 6; i++)
		while (spu[i].sync == 0);

	printf("Output array: ");
	for (i = 0; i < 24; i++)
		printf(" %d", array[i]);
	printf("\n");

	printf("Joining SPU thread group... ");
	printf("%08x\n", sysSpuThreadGroupJoin(group_id, &cause, &status));
	printf("cause=%d status=%d\n", cause, status);

	printf("Closing image... %08x\n", sysSpuImageClose(&image));

	free(array);
	free(spu);

	return 0;
}


int main()
{
	debug_init();

	// SPU test
	spu_test();

	// frame buffer
	void *host_addr = memalign(CB_SIZE, HOST_SIZE);
	init_screen(host_addr, HOST_SIZE);

	// handle program exit
	atexit(program_exit_callback);
	sysUtilRegisterCallback(0,sysutil_exit_callback,NULL);

	// input
	padInfo padinfo;
	padData paddata;
	ioPadInit(7);

	// camera
	camera cam;
	// multisample
	const int sample_per_pixel = 100;
	// ray bounce
	const int max_depth = 50;


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


	// world
	hittable_list world;
	world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
	world.add(make_shared<sphere>(point3(0, -100.5, 0), 100.0));
	
	running = 1;

	// Actually this code starts from the top of the image, as the coordinate of the image is Y pointing upwards,
	// then j = img_height-1 means the top row of the image
	for(int j = img_height-1; j >= 0; --j)
	{
		debug_printf("\rScanline remaining: %d ", j);

		for(int i = 0; i < img_width; ++i)
		{
			color pixel_color(0, 0, 0);
			for (int s = 0; s < sample_per_pixel; ++s)
			{
				sysUtilCheckCallback();
				ioPadGetInfo(&padinfo);
				for(int p=0; p < MAX_PADS; p++)
				{
					if(padinfo.status[p])
					{
						ioPadGetData(p, &paddata);
						if(paddata.BTN_CROSS)
							goto done;
					}
				}

				if (!running)
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

done:
	rsxClearScreenSetBlendState(CLEAR_COLOR);
	blit_to(&bitmap, 0, 0, display_width, display_height, 0, 0, img_width, img_height);
	flip();

	debug_printf("\nDone.\n");

	while(true)
	{
		sysUtilCheckCallback();
		ioPadGetInfo(&padinfo);
		bool pressedExit = false;
		for(int p=0; p < MAX_PADS; p++)
		{
			if(padinfo.status[p]){
				ioPadGetData(p, &paddata);
				if(paddata.BTN_CROSS)
					pressedExit = true;
			}
		}

		if (pressedExit)
			break;
		usleep(2000);
	}
	
	debug_printf("Exiting...\n");
	bitmapDestroy(&bitmap);
	debug_return_ps3loadx();
	return 0;
}

}