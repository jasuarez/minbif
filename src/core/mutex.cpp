/*
 * Copyright(C) 2008 Laurent Defert, Romain Bignon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 *
 */

#include <map>
#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <cstring>
#include "mutex.h"

void Mutex::Init(MutexType _type)
{
	mutex = (pthread_mutex_t*) new pthread_mutex_t;
	type = _type;

	pthread_mutexattr_t attr;
	if(pthread_mutexattr_init(&attr) != 0)
	{
		std::cerr << "pthread_attr_init: " << strerror(errno);
		throw MutexError();
	}

	switch(type)
	{
		case NORMAL_MUTEX:
			//#ifdef DEBUG
			if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) != 0)
			{
				std::cerr << "pthread_mutexattr_settype: " << strerror(errno) << std::endl;
				throw MutexError();
			}
			//#else
			//		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
			//#endif
			break;
		case RECURSIVE_MUTEX:

			if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
			{
				std::cerr << "pthread_mutexattr_settype: " << strerror(errno) << std::endl;
				throw MutexError();
			}
			break;
		default:
			assert(false);
	}

	if(pthread_mutex_init(mutex, &attr) != 0)
	{
		std::cerr << "pthread_mutex_init: " << strerror(errno) << std::endl;
		throw MutexError();
	}
	pthread_mutexattr_destroy(&attr);
}

Mutex::Mutex(MutexType type)
{
	Init(type);
}

Mutex::Mutex(const Mutex& m)
{
	Init(m.type);
}

Mutex& Mutex::operator=(const Mutex& m)
{
	std::cerr << "mutex so bad" << std::endl;
	Init(m.type);
	return *this;
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(mutex);
	delete mutex;
}

void Mutex::Lock() const
{
	int res = pthread_mutex_lock(mutex);
	//assert(res == 0);
	/* Don't use pf_log, because it locks the stdout mutex. */
	if(res)
		std::cerr << "Failed to lock " << this << std::endl;
}

void Mutex::Unlock() const
{
	int res = pthread_mutex_unlock(mutex);
	//assert(res == 0);
	/* Don't use pf_log, because it locks the stdout mutex. */
	if(res)
		std::cerr << "Failed to unlock " << this << std::endl;
}
