#ifndef RWLOCK_H
#define RWLOCK_H

#include <atomic>
#include <cassert>

class RWMutex
{
	std::atomic<unsigned int> m_rw {0};
	static unsigned int readers(unsigned int);
	static unsigned int writing(unsigned int);
	static unsigned int mkrw(unsigned int, unsigned int);
public:
	RWMutex() {};
	void lock_read();
	void unlock_read();
	void lock_write();
	void unlock_write();
	void lock_upgrade();
	void lock_downgrade();
};

class UpgradeLock;

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
