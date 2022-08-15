#include "node_job.h"
#include "scoped_mutex.h"
#include "config.h"
#include "sphere.h"
#include "vec3.h"
#include "debug.h"

#include <sys/mutex.h>
#include <map>
#include <vector>
#include <memory>
#include <malloc.h>
#include <assert.h>

static std::map<uint32_t, node_job_t> s_job_record;
static uint32_t s_job_counter = 0;
static int32_t s_leftover_scanlines;
static int32_t s_curr_frame = -1;
static int32_t s_target_frame = 0;
static int s_img_width, s_img_height;

static sys_mutex_t s_job_mutex;

void init_node_jobs(int img_width, int img_height)
{
	s_img_width = img_width;
	s_img_height = img_height;

	s_leftover_scanlines = img_height;
	s_curr_frame = -1;
	s_target_frame = 0;

	sys_mutex_attr_t mutex_attr;
	sysMutexAttrInitialize(mutex_attr);
	sysMutexCreate(&s_job_mutex, &mutex_attr);
}

void shutdown_node_jobs()
{
	sysMutexDestroy(s_job_mutex);
}

bool allocate_node_job(uint8_t ppu_count, uint8_t spu_count, uint32_t* job_id, uint8_t** serialized_data, size_t* data_size, int* img_width, int* img_height, int* start_y, int* end_y)
{
	scoped_mutex lock(s_job_mutex);

	if (s_leftover_scanlines <= 0)
		return false;

	uint32_t id = ++s_job_counter;
	int32_t capable_take = ppu_count * PPU_CAP + spu_count * SPU_CAP;
	if (s_leftover_scanlines < capable_take)
	{
		capable_take = s_leftover_scanlines;
	}

	std::vector<std::shared_ptr<sphere>> sphere_world;
	// procedural animation based on frame index
	float animate_height = sinf((float)s_target_frame * 10.0f * 0.0174533f) * 0.5f;
	sphere_world.push_back(std::make_shared<sphere>(point3(0, animate_height, -1), 0.5));
	sphere_world.push_back(std::make_shared<sphere>(point3(0, -100.5, 0), 100.0));

	const uint32_t blob_size = 1024;
	uint8_t* obj_blob = (uint8_t*)malloc(blob_size);
	uint8_t element_size = 0;
	uint32_t actual_blob_size = 0;

	// serialise all the spheres
	for(const auto& sphere : sphere_world)
	{
		sphere->serialise(obj_blob + actual_blob_size, &element_size);
		actual_blob_size += element_size;	
	}

	assert(actual_blob_size <= blob_size);
	debug_printf("Sphere blob addr: %lu size: %d actual_size: %d count: %d\n", (uint64_t)obj_blob, blob_size, actual_blob_size, actual_blob_size / element_size);

	node_job_t node_job {
		id,
		s_img_width,
		s_img_height,
		0,
		s_img_width,
		s_img_height - s_leftover_scanlines,
		s_img_height - (s_leftover_scanlines - capable_take)
	};

	s_leftover_scanlines -= capable_take;
	s_job_record[id] = node_job;

	// output
	*job_id = id;
	*serialized_data = obj_blob;
	*data_size = actual_blob_size;
	*img_width = node_job.img_width;
	*img_height = node_job.img_height;
	*start_y = node_job.start_y;
	*end_y = node_job.end_y;

	return true;
}

bool require_more_node_jobs()
{
	scoped_mutex lock(s_job_mutex);
	bool required = s_leftover_scanlines > 0;

	return required;
}


size_t current_node_job_count()
{
	scoped_mutex lock(s_job_mutex);
	size_t num = s_job_record.size();

	return num;
}

bool get_node_job(uint32_t job_id, node_job_t* job)
{
	scoped_mutex lock(s_job_mutex);
	std::map<uint32_t, node_job_t>::iterator it;
	if ((it = s_job_record.find(job_id)) != s_job_record.end())
	{
		*job = (*it).second;
		return true;
	}

	return false;
}

bool dismiss_node_job(uint32_t job_id)
{
	scoped_mutex lock(s_job_mutex);
	bool succeeded = s_job_record.erase(job_id) > 0;

	return succeeded;
}

void complete_curr_frame()
{
	s_curr_frame++;
}

int32_t get_curr_frame()
{
	return s_curr_frame;
}

bool is_rendering_curr_frame()
{
	return s_curr_frame < s_target_frame;
}

void advance_next_frame()
{
	scoped_mutex lock(s_job_mutex);
	s_leftover_scanlines = s_img_height;
	s_target_frame++;
}