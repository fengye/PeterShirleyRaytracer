#ifndef _H_SCOPED_MUTEX_
#define _H_SCOPED_MUTEX_

#include <sys/mutex.h>

class scoped_mutex
{
	sys_mutex_t _mutex;
public:
	scoped_mutex(sys_mutex_t mutex) : _mutex(mutex)
	{
		sysMutexLock(_mutex, 0);
	}

	~scoped_mutex()
	{
		sysMutexUnlock(_mutex);
	}
};

#endif //_H_SCOPED_MUTEX_