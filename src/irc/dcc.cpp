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
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "dcc.h"
#include "nick.h"
#include "message.h"
#include "../util.h"
#include "../log.h"

namespace irc {

DCCSend::DCCSend(const im::FileTransfert& _ft, Nick* _nick)
	: ft(_ft),
	  total_size(_ft.getSize()),
	  filename(_ft.getFileName()),
	  local_filename(_ft.getLocalFileName()),
	  nick(_nick),
	  listen_data(NULL),
	  watcher(0),
	  fd(-1),
	  port(0),
	  finished(false),
	  bytes_sent(0),
	  fp(NULL),
	  rxlen(0),
	  rxqueue(NULL)
{
	listen_data = purple_network_listen_range(0, 0, SOCK_STREAM, &DCCSend::listen_cb, this);
	if(!listen_data)
		throw DCCListenError();
}

DCCSend::~DCCSend()
{
	deinit();
}

void DCCSend::deinit()
{
	finished = true;
	if(fd >= 0)
		close(fd);
	if(fp != NULL)
		fclose(fp);
	if(watcher > 0)
		purple_input_remove(watcher);
	if(listen_data != NULL)
		purple_network_listen_cancel(listen_data);
	g_free(rxqueue);
}

void DCCSend::dcc_send()
{
	if(finished || listen_data || fd < 0)
		return;

	if(!fp)
	{
		fp = fopen(local_filename.c_str(), "r");
		if(!fp)
			return; /* File isn't written yet. */
	}

	/* It does not add any \0 on buffer */
	static char buf[512];
	size_t len = fread(buf, sizeof(char), sizeof buf, fp);

	if(len > 0)
		send(fd, buf, len, 0);

	bytes_sent += len;
}

void DCCSend::updated(bool destroy)
{
	if(destroy)
		ft = im::FileTransfert(); /* No-valid object */

	dcc_send();
}

void DCCSend::dcc_read(gpointer data, int source, PurpleInputCondition cond)
{
	DCCSend* dcc = static_cast<DCCSend*>(data);
	static char buffer[64];
	ssize_t len;

	len = read(source, buffer, sizeof(buffer));

	if (len < 0 && errno == EAGAIN)
		return;
	else if (len <= 0) {
		dcc->fd = -1;
		dcc->deinit();
		return;
	}

	dcc->rxqueue = (guchar*)g_realloc(dcc->rxqueue, len + dcc->rxlen);
	memcpy(dcc->rxqueue + dcc->rxlen, buffer, len);
	dcc->rxlen += (guint)len;

	while (1) {
		size_t acked;

		if (dcc->rxlen < 4)
			break;

		acked = ntohl(*((gint32 *)dcc->rxqueue));

		dcc->rxlen -= 4;
		if (dcc->rxlen) {
			unsigned char *tmp = (unsigned char*)g_memdup(dcc->rxqueue + 4, (guint)dcc->rxlen);
			g_free(dcc->rxqueue);
			dcc->rxqueue = tmp;
		} else {
			g_free(dcc->rxqueue);
			dcc->rxqueue = NULL;
		}

		if (acked >= dcc->total_size) {
			dcc->deinit();
			return;
		}
	}

	dcc->dcc_send();
}

void DCCSend::connected(gpointer data, int source, PurpleInputCondition cond)
{
	DCCSend* dcc = static_cast<DCCSend*>(data);
	int conn = accept(dcc->fd, NULL, 0), flags;
	if(!conn)
	{
		b_log[W_ERR] << "DCC connection failed";
		dcc->deinit();
		return;
	}

	purple_input_remove(dcc->watcher);
	dcc->watcher = 0;
	close(dcc->fd);
	dcc->fd = conn;
	dcc->listen_data = NULL;

	flags = fcntl(conn, F_GETFL);
	fcntl(conn, F_SETFL, flags | O_NONBLOCK);
	fcntl(conn, F_SETFD, FD_CLOEXEC);
	dcc->watcher = purple_input_add(conn, PURPLE_INPUT_READ, DCCSend::dcc_read, dcc);

	dcc->dcc_send();
}

void DCCSend::listen_cb(int sock, void* data)
{
	DCCSend* dcc = static_cast<DCCSend*>(data);
	struct in_addr addr;

	dcc->fd = sock;
	dcc->port = purple_network_get_port_from_fd(sock);
	inet_aton(purple_network_get_my_ip(-1), &addr);

	dcc->watcher = purple_input_add(sock, PURPLE_INPUT_READ,
					connected, dcc);

	dcc->nick->send(Message(MSG_PRIVMSG).setSender("romain_")
			                 .setReceiver(dcc->nick)
					 .addArg("\001DCC SEND \"" + dcc->filename + "\" " +
						 t2s(ntohl(addr.s_addr)) + " " +
						 t2s(dcc->port) + " " +
						 t2s(dcc->total_size) + "\001"));
}

} /* namespace irc */
