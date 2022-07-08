#ifndef _H_JOB_
#define _H_JOB_

#include "spu_shared.h"
struct job_t
{
	virtual void print_job_details() const = 0;
	virtual world_data_t* get_input() = 0;
	virtual pixel_data_t* get_output() = 0;
	virtual int8_t get_processor_index() const = 0;
};

#endif //_H_JOB_