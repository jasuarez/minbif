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

#include <sys/socket.h>
#include <netdb.h>

#include "../log.h"
#include "irc.h"
#include "message.h"

IRC::IRC(int _fd)
	: fd(_fd),
	  hostname("localhost.localdomain")
{
	struct sockaddr_storage sock;
	socklen_t socklen = sizeof(sock);

	if(getsockname(fd, (struct sockaddr*) &sock, &socklen) == 0)
	{
		char buf[NI_MAXHOST+1];

		if( getnameinfo((struct sockaddr *) &sock, socklen, buf, NI_MAXHOST, NULL, 0, 0 ) == 0)
		{
			hostname = buf;
		}
	}
	sendmsg(Message(MSG_NOTICE).setSender(this).setReceiver("AUTH").addArg("BitlBee-IRCd initialized, please go on"));
}

IRC::~IRC()
{
}

bool IRC::sendmsg(Message msg)
{
	string s = msg.format();
	write(fd, s.c_str(), s.size());

	return true;
}
