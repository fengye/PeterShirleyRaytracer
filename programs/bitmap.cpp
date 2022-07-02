#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <ppu-types.h>

#include <rsx/rsx.h>

#include "bitmap.h"

void bitmapInit(Bitmap *bitmap, u32 width, u32 height) {
  s32 status;
  bitmap->pixels = (u32*)rsxMemalign(64, width * height * 4);
  status = rsxAddressToOffset(bitmap->pixels, &bitmap->offset);
  assert(status==0);
  bitmap->width = width;
  bitmap->height = height;
}

void bitmapDestroy(Bitmap *bitmap) {
  rsxFree(bitmap->pixels);
  bitmap->pixels = NULL;
}

void bitmapSetXpm(Bitmap *bitmap, const char* xpm[]) {
  u32 palette[256];
  u32 width, height, ncolors, depth;
  char *p;
  u32 ln;
  u32 x, y, *pix;

  width = atoi(xpm[0]);
  height = atoi(p = strchr(xpm[0], ' ') + 1);
  ncolors = atoi(p = strchr(p, ' ') + 1);
  depth = atoi(p = strchr(p, ' ') + 1);
  assert(depth == 1);

  bitmapInit(bitmap, width, height);

  /* read palette */
  memset(palette, 0, sizeof(palette));
  for (ln = 1; ln <= ncolors; ++ln) {
    u32 color = strtol(&xpm[ln][5], NULL, 16);
    /* black color is transparent, others are opaque */
    if (color != 0)
      color |= 0xff000000;
    palette[(unsigned char)xpm[ln][0]] = color;
  }

  /* convert image */
  pix = bitmap->pixels;
  for (y = 0; y < height; ++y, ++ln)
    for (x = 0; x < width; ++x)
      *pix++ = palette[(unsigned char)xpm[ln][x]];
}

void bitmapUpdateRGBA(Bitmap* bitmap, u32 x, u32 y, u8 r, u8 g, u8 b, u8 a)
{
  assert(x >= 0 && x < bitmap->width);
  assert(y >= 0 && y < bitmap->height);

  bitmap->pixels[y * bitmap->width + x] = a << 24 | r << 16 | g << 8 | b;
}

