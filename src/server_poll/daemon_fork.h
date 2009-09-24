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
};
class _CallBack;

class DaemonForkServerPoll : public ServerPoll
{
	irc::IRC* irc;
	int sock;
	int read_id;
	_CallBack *read_cb;
	_CallBack* stop_cb;

public:

	DaemonForkServerPoll(Minbif* application);
	~DaemonForkServerPoll();

	bool new_client_cb(void*);

	void kill(irc::IRC* irc);
	bool stopServer_cb(void*);

	void log(size_t level, string log) const;
};

#endif /* SERVER_POLL_DAEMON_FORK_H */
