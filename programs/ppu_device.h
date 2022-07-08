#include "spu_shared.h"
#include "job.h"
#include "ppu_job.h"
#include <sys/thread.h>
#include <sys/mutex.h>
#include <sys/cond.h>
#include <cstdio>

extern void ppu_raytrace(void* ppu_job);

struct ppu_device_t
{
	ppu_job_t* ppu_job = NULL;
	uint32_t ppu_index = 0xff;
	sys_ppu_thread_t thread_id = 0;
	char threadname[128];
	sys_cond_t cond_var;
	sys_mutex_t mutex;
	uint32_t sync = SYNC_INVALID; // just like in spu_shared_t::sync

	ppu_device_t(uint32_t ppu_index_)
	{
		ppu_index = ppu_index_;

		// create mutex
		sys_mutex_attr_t mutex_attr;
		sysMutexAttrInitialize(mutex_attr);
		sysMutexCreate(&mutex, &mutex_attr);

		// create conditional variable
		sys_cond_attr cond_attr;
		sysCondAttrInitialize(cond_attr);
		sysCondCreate(&cond_var, mutex, &cond_attr);
	}

	void update_job(job_t* ppu_job_)
	{
		ppu_job = (ppu_job_t*)ppu_job_;
	}

	int32_t pre_kickoff()
	{
		// actually start the thread already
		u64 priority = 1500;
		size_t stacksize = 0x8000;
		sprintf(threadname, "ppu_rt %d", ppu_index);

		return sysThreadCreate(&thread_id, ppu_raytrace, this, priority, stacksize, THREAD_JOINABLE, threadname);
	}

	uint32_t signal_start()
	{
		sysMutexLock(mutex, 0);

		sync = SYNC_START;
		sysCondSignal(cond_var);

		sysMutexUnlock(mutex);
		return 0;
	}

	uint32_t signal_quit()
	{
		sysMutexLock(mutex, 0);
		sync = SYNC_QUIT;
		sysCondSignal(cond_var);

		sysMutexUnlock(mutex);
		return 0;
	}

	uint32_t check_sync() const
	{
		return sync;
	}

	~ppu_device_t()
	{
		sysCondDestroy(cond_var);
		sysMutexDestroy(mutex);
	}
};
