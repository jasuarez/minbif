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
	/** IPC child data structure */
	struct child_t
	{
		int fd;
		int read_id;
		_CallBack* read_cb;
	};

	/** IPC commands array. */
	static struct ipc_cmds_t
	{
		const char* cmd;
		void (DaemonForkServerPoll::*func) (child_t* child, irc::Message m);
		unsigned min_args;
	} ipc_cmds[];

	/** \page IPC
	 *
	 * Daemon fork mode forks everytimes there is a new connection.
	 *
	 * Communication between children and parent is made with two sockets
	 * that are shared. Every commands are formatted like an IRC command,
	 * and the irc::Message class can be used to parse or format commands.
	 * Note that it is forbidden to set a sender or a receiver.
	 *
	 */
	void m_wallops(child_t* child, irc::Message m);     /**< IPC handler for the WALLOPS command. */
	void m_rehash(child_t* child, irc::Message m);      /**< IPC handler for the REHASH command. */
	void m_die(child_t* child, irc::Message m);         /**< IPC handler for the DIE command. */
	void m_oper(child_t* child, irc::Message m);        /**< IPC handler for the OPER command. */

	irc::IRC* irc;
	int maxcon;
	int sock;
	int read_id;
	_CallBack *read_cb;
	vector<child_t*> childs;

	bool ipc_read(void*);

	/** Master sends a IPC message to a child.
	 *
	 * @param child  child data structure
	 * @param m  message to send
	 * @return  true if the message has correctly been sent.
	 */
	bool ipc_master_send(child_t* child, const irc::Message& m);

	/** Master broadcasts a IPC message to every children.
	 *
	 * @param m  message to send
	 * @param butone  optional argument. If != NULL, the message is sent
	 *                to everybody except this one
	 * @return  true if the message has correctly been sent to at least
	 *               one child
	 */
	bool ipc_master_broadcast(const irc::Message& m, child_t* butone = NULL);

	/** Child send a message to his master.
	 *
	 * @param m  message to send
	 * @return  true if the message has correctly been sent.
	 */
	bool ipc_child_send(const irc::Message& m);

public:

	DaemonForkServerPoll(Minbif* application, ConfigSection* _config);
	~DaemonForkServerPoll();

	bool new_client_cb(void*);

	void rehash();
	void kill(irc::IRC* irc);
	bool stopServer_cb(void*);
	bool ipc_send(const irc::Message& msg);

	void log(size_t level, string log) const;
};

#endif /* SERVER_POLL_DAEMON_FORK_H */
