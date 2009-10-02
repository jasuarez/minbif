/*
 * Copyright(C) 2009 Romain Bignon
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

#include <cstdio>
#include "user.h"

namespace irc {

User::User(int _fd, Server* server, string nickname, string identname, string hostname, string realname)
	: Nick(server, nickname, identname, hostname, realname),
	  fd(_fd)
{
	if(fileno(stdin) == fd)
		fd = fileno(stdout);
}

User::~User()
{
}

void User::send(Message msg)
{
	if(fd >= 0)
	{
		string s = msg.format();
		write(fd, s.c_str(), s.size());
	}
}

void User::setLastReadNow()
{
	last_read = time(NULL);
}

}; /* namespace irc */
