#include "debug.h"

#include "raytracer.h"
#include "color.h"
#include "ray.h"
#include "vec3.h"
#include "sphere.h"
#include "hittable_list.h"
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

color ray_color(const ray& r, const hittable& world)
{
	hit_record record;
	if (world.hit(r, 0, RT_infinity, record))
	{
		return 0.5 * (record.normal + color(1, 1, 1));
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

void blit_scale(Bitmap *bitmap, u32 dstX, u32 dstY, u32 srcX, u32 srcY, u32 w, u32 h, float zoom)
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
  scale.outW = w * zoom;
  scale.outH = h * zoom;
  scale.ratioX = rsxGetFixedSint32(1.f / zoom);
  scale.ratioY = rsxGetFixedSint32(1.f / zoom);
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
	padInfo padinfo;
	padData paddata;
	ioPadInit(7);

	// img
	const auto aspect_ratio = 16.0 / 9.0;
	const int img_width = 400;
	const int img_height = (int)(img_width / aspect_ratio);

	debug_printf("Image %dx%d\n", img_width, img_height);

	// ps3 bitmap
	Bitmap bitmap;
	bitmapInit(&bitmap, img_width, img_height);
	setRenderTarget(curr_fb);

	// clear screen
	const u32 CLEAR_COLOR = 0x20FF30;
	rsxClearScreenSetBlendState(CLEAR_COLOR);
    flip();

	// camera
	const auto viewport_height = 2.0;
	const auto viewport_width = viewport_height * aspect_ratio;
	const auto focal_length = 1.0;

	const point3 origin(0.0, 0.0, 0.0);
	const vec3 horizontal(viewport_width, 0.0, 0.0);
	const vec3 vertical(0.0, viewport_height, 0.0);
	vec3 lower_left_corner = origin - horizontal / 2.0 - vertical / 2.0 + vec3(0, 0, -focal_length);

	// world
	hittable_list world;
	world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
	world.add(make_shared<sphere>(point3(0, -100.5, 0), 100.0));
	
	running = 1;

	// Actually this code starts from the top of the image, as the coordinate of the image is Y pointing upwards,
	// then j = img_height-1 means the top row of the image
	for(int j = img_height-1; j >= 0; --j)
	{
		fprintf(stderr, "\rScanline remaining: %d ", j);
		fflush(stderr);

		for(int i = 0; i < img_width; ++i)
		{
			sysUtilCheckCallback();
			ioPadGetInfo(&padinfo);
			for(int p=0; p < MAX_PADS; p++)
			{
				if(padinfo.status[p]){
					ioPadGetData(p, &paddata);
					if(paddata.BTN_CROSS)
						goto done;
				}
			}

			if (!running)
				goto done;

			double u = (double)i / (img_width - 1);
			double v = (double)j / (img_height - 1);

			ray r(origin, lower_left_corner + u * horizontal + v * vertical - origin);
			color pixel_color = ray_color(r, world);
			// Bitmap using origin on top-left, with Y axis pointing down, so have to do the conversion here
			write_color_bitmap(&bitmap, i, (img_height - (j+1)), pixel_color);
		}

		if (j % 10 == 0)
		{
			rsxClearScreenSetBlendState(CLEAR_COLOR);
			blit_scale(&bitmap, 0, 0, 0, 0, img_width, img_height, 4.0f);
			flip();
		}
	}

done:
	rsxClearScreenSetBlendState(CLEAR_COLOR);
	blit_scale(&bitmap, 0, 0, 0, 0, img_width, img_height, 4.0f);
	flip();

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
	debug_printf("\nDone.\n");

	bitmapDestroy(&bitmap);

	debug_return_ps3loadx();
	return 0;
}

}