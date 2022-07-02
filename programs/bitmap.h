#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <ppu-types.h>

typedef struct {
  u32 width;
  u32 height;
  u32 *pixels;
  u32 offset;
} Bitmap;

void bitmapInit(Bitmap *bitmap, u32 width, u32 height);

void bitmapDestroy(Bitmap *bitmap);

void bitmapSetXpm(Bitmap *bitmap, const char* xpm[]);

void bitmapUpdateRGBA(Bitmap* bitmap, u32 x, u32 y, u8 r, u8 g, u8 b, u8 a);

#endif // __BITMAP_H__