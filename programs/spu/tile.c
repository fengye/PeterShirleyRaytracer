#include "tile.h"


static int32_t s_tiles_horz; // how many tiles horizontally
static int32_t s_tiles_vert; // vertically
static int s_img_width, s_img_height;

void init_tiles(int img_width, int img_height)
{
	s_img_width = img_width;
	s_img_height = img_height;

	s_tiles_horz = (img_width + TILE_WIDTH - 1) / TILE_WIDTH;
	s_tiles_vert = (img_height + TILE_HEIGHT - 1) / TILE_HEIGHT;
}

int get_total_tiles_count()
{
	return s_tiles_horz * s_tiles_vert;
}

int get_horizontal_tiles_count()
{
	return s_tiles_horz;
}

int get_vertical_tiles_count()
{
	return s_tiles_vert;
}

// bounding box values are inclusive. start = 0, end = 15 means 16 pixels to be processed
void calc_tile_bounding_box(int tile_id, int* out_start_x, int* out_start_y, int* out_end_x, int* out_end_y)
{
	*out_start_x =  (tile_id % s_tiles_horz) * TILE_WIDTH;
	int end_x = *out_start_x + TILE_WIDTH - 1; // like [0, 15] or [16, 31]
	if (end_x > s_img_width - 1)
	{
		end_x = s_img_width - 1;
	}
	*out_end_x = end_x;

	*out_start_y = (tile_id / s_tiles_horz) * TILE_HEIGHT;
	int end_y = *out_start_y + TILE_HEIGHT - 1;
	if (end_y > s_img_height - 1)
	{
		end_y = s_img_height - 1;
	}
	*out_end_y = end_y;
}


void get_tile_coord(int tile_id, int* out_left_pos, int* out_top_pos)
{
	int dummy1, dummy2;
	calc_tile_bounding_box(tile_id, out_left_pos, out_top_pos, &dummy1, &dummy2);
}

// copy the pixel out version
void get_local_pixel_from_tile_offset(pixel_data_t* local_pixel_data, int tile_id_offset, int local_x, int local_y, pixel_data_t* out_pixel)
{
 	pixel_data_t* curr_tile_pixel_data = local_pixel_data + ((TILE_WIDTH * TILE_HEIGHT) * tile_id_offset); // pointer type of pixel_data_t so do not multiply 4

 	*out_pixel = curr_tile_pixel_data[local_y * TILE_WIDTH + local_x];
}

// retrieve the pixel pointer version
pixel_data_t* get_local_pixel_pointer_from_tile_offset(pixel_data_t* local_pixel_data, int tile_id_offset, int local_x, int local_y)
{
 	pixel_data_t* curr_tile_pixel_data = local_pixel_data + ((TILE_WIDTH * TILE_HEIGHT) * tile_id_offset);

 	return curr_tile_pixel_data + (local_y * TILE_WIDTH + local_x);
}