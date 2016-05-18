#ifndef RWLOCK_H
#define RWLOCK_H

#include <atomic>

class RWMutex
{
	std::atomic<unsigned int> m_rw {0};
	static unsigned int readers(unsigned int);
	static unsigned int writing(unsigned int);
	static unsigned int mkrw(unsigned int, unsigned int);
public:
	RWMutex() {};
	void lock_read(); // acquire shared ownership
	void unlock_read(); // release shared ownership
	void lock_write(); // acquire unique ownership
	void unlock_write(); // release unique ownership
	void lock_upgrade(); // switch from shared to unique ownership
	void lock_downgrade(); // switch from unique to shared ownership
};

class UpgradeLock;

// acquire shared ownership of mutex, automatically releasing when destroyed
class ReadLock
{
	friend UpgradeLock;
	RWMutex& m_mutex;
public:
	ReadLock(RWMutex& mutex)
		: m_mutex {mutex}
	{
		m_mutex.lock_read();
	}

	~ReadLock()
	{
		m_mutex.unlock_read();
	}
};

// acquire unique ownership of mutex, automatically releasing when destroyed
class WriteLock
{
	RWMutex& m_mutex;
public:
	WriteLock(RWMutex& mutex)
		: m_mutex {mutex}
	{
		m_mutex.lock_write();
	}

	~WriteLock()
	{
		m_mutex.unlock_write();
	}
};

// upgrade lock to unique ownership, automatically reverting to shared when destroyed
class UpgradeLock
{
	ReadLock& m_lock;
public:
	UpgradeLock(ReadLock& lock)
		: m_lock {lock}
	{
		m_lock.m_mutex.lock_upgrade();
	}

	~UpgradeLock()
	{
		m_lock.m_mutex.lock_downgrade();
	}
};

#endif

