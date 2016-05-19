#ifndef PLOCK_H
#define PLOCK_H

#include <pthread.h>

class PMutex
{
	pthread_rwlock_t m_rw;
public:
	PMutex();
	~PMutex();
	void lock_read(); // acquire shared ownership
	void unlock_read(); // release shared ownership
	void lock_write(); // acquire unique ownership
	void unlock_write(); // release unique ownership
};

// acquire shared ownership of mutex, automatically releasing when destroyed
class PReadLock
{
	PMutex& m_mutex;
public:
	PReadLock(PMutex& mutex)
		: m_mutex {mutex}
	{
		m_mutex.lock_read();
	}

	~PReadLock()
	{
		m_mutex.unlock_read();
	}
};

// acquire unique ownership of mutex, automatically releasing when destroyed
class PWriteLock
{
	PMutex& m_mutex;
public:
	PWriteLock(PMutex& mutex)
		: m_mutex {mutex}
	{
		m_mutex.lock_write();
	}

	~PWriteLock()
	{
		m_mutex.unlock_write();
	}
};

#endif
