/*
 * Minbif - IRC instant messaging gateway
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

#ifndef SERVER_POLL_DAEMON_FORK_H
#define SERVER_POLL_DAEMON_FORK_H

#include <vector>

#include "poll.h"

namespace irc {
	class IRC;
	class Message;
};

class _CallBack;
using std::vector;

class DaemonForkServerPoll : public ServerPoll
{
	struct child_t
	{
		int fd;
		int read_id;
		_CallBack* read_cb;
	};

	static struct ipc_cmds_t
	{
		const char* cmd;
		void (DaemonForkServerPoll::*func) (child_t* child, irc::Message m);
	} ipc_cmds[];

	void m_wallops(child_t* child, irc::Message m);
	void m_rehash(child_t* child, irc::Message m);

	irc::IRC* irc;
	int sock;
	int read_id;
	_CallBack *read_cb;
	vector<child_t*> childs;

	bool ipc_read(void*);
	void ipc_master_send(child_t* child, const irc::Message& m);
	void ipc_master_broadcast(const irc::Message& m);
	void ipc_child_send(const irc::Message& m);

public:

	DaemonForkServerPoll(Minbif* application);
	~DaemonForkServerPoll();

	bool new_client_cb(void*);

	void rehash();
	void kill(irc::IRC* irc);
	bool stopServer_cb(void*);
	bool ipc_send(const irc::Message& msg);

	void log(size_t level, string log) const;
};

#endif /* SERVER_POLL_DAEMON_FORK_H */
