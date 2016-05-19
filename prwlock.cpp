#include "prwlock.h"

PMutex::PMutex()
{
	pthread_rwlock_init(&m_rw, nullptr);
}

PMutex::~PMutex()
{
	pthread_rwlock_destroy(&m_rw);
}

void PMutex::lock_read()
{
	pthread_rwlock_rdlock(&m_rw);
}

void PMutex::unlock_read()
{
	pthread_rwlock_unlock(&m_rw);
}

void PMutex::lock_write()
{
	pthread_rwlock_wrlock(&m_rw);
}

void PMutex::unlock_write()
{
	pthread_rwlock_unlock(&m_rw);
}
