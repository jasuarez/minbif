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

#include "sockwrap.h"
#include "sockwrap_plain.h"
#ifdef HAVE_TLS
#  include "sockwrap_tls.h"
#endif
#include "core/config.h"
#include "core/util.h"

SockWrapper::SockWrapper(int _fd) : fd(_fd)
{
}

SockWrapper::~SockWrapper()
{
}

SockWrapper* SockWrapper::Builder(int _fd)
{
#ifdef HAVE_TLS
	if (conf.GetSection("daemon")->GetItem("security")->String().compare("ssl") == 0)
		return new SockWrapperTLS(_fd);
#endif
	return new SockWrapperPlain(_fd);
}

string SockWrapper::GetClientHostname()
{
	struct sockaddr_storage sock;
	socklen_t socklen = sizeof(sock);

	/* Get the client's hostname. */
	string clienthost = "localhost.localdomain";
	if(getpeername(fd, (struct sockaddr*) &sock, &socklen) == 0)
	{
		char buf[NI_MAXHOST+1];

		if(getnameinfo((struct sockaddr *)&sock, socklen, buf, NI_MAXHOST, NULL, 0, 0) == 0)
			clienthost = buf;
	}

	return clienthost;
}

string SockWrapper::GetServerHostname()
{
	struct sockaddr_storage sock;
	socklen_t socklen = sizeof(sock);

	/* Get the server's hostname. */
	string serverhost = "localhost.localdomain";
	if(getsockname(fd, (struct sockaddr*) &sock, &socklen) == 0)
	{
		char buf[NI_MAXHOST+1];

		if(getnameinfo((struct sockaddr *) &sock, socklen, buf, NI_MAXHOST, NULL, 0, 0 ) == 0)
			serverhost = buf;
	}

	return serverhost;
}

int SockWrapper::AttachCallback(PurpleInputCondition cond, _CallBack* cb)
{
	return glib_input_add(fd, cond, g_callback_input, cb);
}

