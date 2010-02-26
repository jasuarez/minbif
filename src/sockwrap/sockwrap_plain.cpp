/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon, Marc Dequènes (Duck)
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
 */

#include "sockwrap_plain.h"
#include "sock.h"
#include "log.h"
#include <sys/socket.h>
#include <cstring>

SockWrapperPlain::SockWrapperPlain(int _fd) : SockWrapper(_fd)
{
}

SockWrapperPlain::~SockWrapperPlain()
{
}

string SockWrapperPlain::Read()
{
	static char buf[1024];
	string sbuf;
	ssize_t r;

	if((r = read(fd, buf, sizeof buf - 1)) <= 0)
	{
		if(r == 0)
			throw SockError::SockError("Connection reset by peer...");
		else if(!sockerr_again())
			throw SockError::SockError(string("Read error: ") + strerror(errno));
		else
			sbuf = "";
	}
	else
	{
		buf[r] = 0;
		sbuf = buf;
	}

	return sbuf;
}

