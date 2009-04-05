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
#include "nick.h"

IRC::IRC(int _fd, string _hostname)
	: fd(_fd),
	  hostname("localhost.localdomain"),
	  userNick(NULL)
{
	struct sockaddr_storage sock;
	socklen_t socklen = sizeof(sock);

	/* Get the user's hostname. */
	string userhost = "localhost.localdomain";
	if(getpeername(fd, (struct sockaddr*) &sock, &socklen) == 0)
	{
		char buf[NI_MAXHOST+1];

		if(getnameinfo((struct sockaddr *)&sock, socklen, buf, NI_MAXHOST, NULL, 0, 0) == 0)
			userhost = buf;
	}

	userNick = new Nick("AUTH", "auth", userhost);

	/* Get my own hostname (if not given in arguments) */
	if(_hostname.empty() || _hostname == " ")
	{
		if(getsockname(fd, (struct sockaddr*) &sock, &socklen) == 0)
		{
			char buf[NI_MAXHOST+1];

			if( getnameinfo((struct sockaddr *) &sock, socklen, buf, NI_MAXHOST, NULL, 0, 0 ) == 0)
				hostname = buf;
		}
	}
	else if(_hostname.find(" ") != string::npos)
	{
		/* An hostname can't contain any space. */
		b_log[W_ERR] << "'" << _hostname << "' is not a valid server hostname";
		throw IRCAuthError();
	}
	else
		hostname = _hostname;

	sendmsg(Message(MSG_NOTICE).setSender(this).setReceiver(userNick).addArg("BitlBee-IRCd initialized, please go on"));
}

IRC::~IRC()
{
	delete userNick;
}

bool IRC::sendmsg(Message msg) const
{
	string s = msg.format();
	write(fd, s.c_str(), s.size());

	return true;
}
