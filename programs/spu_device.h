#ifndef _H_SPU_DEVICE_
#define _H_SPU_DEVICE_

#include "spu_shared.h"
#include "spu_job.h"

struct spu_device_t
{
	spu_shared_t* spu = nullptr;

	spu_device_t(uint32_t spu_index)
	{
		spu = (spu_shared_t*)memalign(SPU_ALIGN, sizeof(spu_shared_t));
		spu->rank = spu_index;
		spu->count = SPU_COUNT;
		spu->sync = SYNC_INVALID;
		// uninitialised job data
		spu->i_data_ea = 0;
		spu->i_data_sz = 0;
		spu->o_data_ea = 0;
		spu->o_data_sz = 0;
	}

	void update_job(spu_job_t* spu_job)
	{
		world_data_t* input = spu_job->input;
		pixel_data_t* output = spu_job->output;

		spu->i_data_ea = ptr2ea(input);
		spu->i_data_sz = sizeof(world_data_t);
		spu->o_data_ea = ptr2ea(output);
		spu->o_data_sz = spu_job->output_size;
	}

	uint32_t pre_kickoff(sysSpuImage* image, sysSpuThreadAttribute* attr, uint32_t group_id)
	{
		uint32_t spuRet = 0;
		sysSpuThreadArgument arg;
		arg.arg0 = ptr2ea(spu);

		debug_printf("Creating SPU thread... ");
		spuRet = sysSpuThreadInitialize(&spu->id, group_id, spu->rank, image, attr, &arg);
		debug_printf("%08x\n", spuRet);
		debug_printf("thread id = %d\n", spu->id);

		debug_printf("Configuring SPU... ");
		spuRet = sysSpuThreadSetConfiguration(spu->id, SPU_SIGNAL1_OVERWRITE|SPU_SIGNAL2_OVERWRITE);
		debug_printf("%08x\n", spuRet);

		return spuRet;
	}

	uint32_t signal_start()
	{
		spu->sync = SYNC_START;
		return sysSpuThreadWriteSignal(spu->id, 0, 1);
	}

	uint32_t signal_quit()
	{
		spu->sync = SYNC_QUIT;
		return sysSpuThreadWriteSignal(spu->id, 0, 1);
	}

	uint32_t check_sync() const
	{
		return spu->sync;
	}

	~spu_device_t()
	{
		free(spu);
	}
};


#endif //_H_SPU_DEVICE_