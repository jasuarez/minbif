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

#ifndef SERVER_POLL_INETD_H
#define SERVER_POLL_INETD_H

#include "poll.h"

class IRC;
class _CallBack;

class InetdServerPoll : public ServerPoll
{
	IRC* irc;
	_CallBack* stop_cb;

public:

	InetdServerPoll(Bitlbee* application);
	~InetdServerPoll();

	void kill(IRC* irc);
	bool stopServer_cb(void*);

	void log(string log) const;
};

#endif /* SERVER_POLL_INETD_H */