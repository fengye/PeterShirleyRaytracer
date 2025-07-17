#ifndef _H_TILE_
#define _H_TILE_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "spu_shared.h"

#define TILE_WIDTH (16)
#define TILE_HEIGHT (16)

extern void init_tiles(int img_width, int img_height);
extern int get_total_tiles_count();
extern int get_horizontal_tiles_count();
extern int get_vertical_tiles_count();
extern void calc_tile_bounding_box(int tile_id, int* out_start_x, int* out_start_y, int* out_end_x, int* out_end_y);
extern void get_tile_coord(int tile_id, int* out_left_pos, int* out_top_pos);
extern void get_local_pixel_from_tile_offset(pixel_data_t* local_pixel_data, int tile_id_offset, int x, int y, pixel_data_t* out_pixel);
extern pixel_data_t* get_local_pixel_pointer_from_tile_offset(pixel_data_t* local_pixel_data, int tile_id_offset, int x, int y); // esstenaily the same but return a pointer rather than copy pixel data out
#endif //_H_TILE_