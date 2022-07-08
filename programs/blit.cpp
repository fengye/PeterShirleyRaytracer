#include "bitmap.h"
#include <rsx/rsx.h>
#include "rsxutil.h"

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