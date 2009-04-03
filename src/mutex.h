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

#ifndef MUTEX_H
#define MUTEX_H
#include <pthread.h>
#include <exception>

enum MutexType
{
	INVALID_MUTEX,
	NORMAL_MUTEX,
	RECURSIVE_MUTEX
};

class Mutex
{
private:
	pthread_mutex_t* mutex;

	MutexType type;

	void Init(enum MutexType _type);

public:

	class MutexError : public std::exception {};

	Mutex(enum MutexType _type = NORMAL_MUTEX);
	Mutex(const Mutex &m);
	Mutex& operator=(const Mutex& m);
	~Mutex();

	void Lock() const;
	void Unlock() const;
};

/** Lock a Mutex locally.
 *
 * On a block, juste create an auto instance of this class.
 * When it will be removed (while exiting this block), mutex will be unlocked.
 *
 * Juste use:
 * BlockLockMutex lock(mutex);
 *
 */
class BlockLockMutex
{
	const Mutex* p;
	BlockLockMutex(const BlockLockMutex&);
	BlockLockMutex& operator=(const BlockLockMutex&);

public:
	explicit BlockLockMutex(const Mutex* _p)
		: p(_p)
	{
		p->Lock();
	}
	~BlockLockMutex()
	{
		p->Unlock();
	}
};
#endif
