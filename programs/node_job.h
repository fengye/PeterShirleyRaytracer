#ifndef _H_NODE_JOB_
#define _H_NODE_JOB_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct _node_job_t
{
	uint32_t job_id;
	int img_width;
	int img_height;
	int start_x;
	int end_x;
	int start_y;
	int end_y;
} node_job_t;

extern void init_node_jobs(int img_width, int img_height);
extern void shutdown_node_jobs();
extern bool require_more_node_jobs();
extern size_t current_node_job_count();
extern bool allocate_node_job(uint8_t ppu_count, uint8_t spu_count, uint32_t* job_id, uint8_t** serialized_data, size_t* data_size, int* img_width, int* img_height, int* start_y, int* end_y);
extern bool get_node_job(uint32_t job_id, node_job_t* job);
extern bool dismiss_node_job(uint32_t job_id);

#endif //_H_NODE_JOB_