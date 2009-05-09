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

#ifndef SERVER_POLL_H
#define SERVER_POLL_H

#include <exception>
#include <string>

using std::string;

class Minbif;

namespace irc
{
	class IRC;
};

class ServerPollError : public std::exception {};

class ServerPoll
{
	Minbif* application;

protected:

	Minbif* getApplication() const { return application; }

public:

	enum poll_type_t
	{
		INETD,
		DAEMON,
		DAEMON_FORK
	};

	static ServerPoll* build(poll_type_t type, Minbif* application);

	ServerPoll(Minbif* application);

	virtual void kill(irc::IRC* irc) = 0;

	virtual void log(size_t level, string string) const = 0;
};

#endif /* SERVER_POLL_H */
