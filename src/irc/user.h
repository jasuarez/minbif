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

#ifndef IRC_USER_H
#define IRC_USER_H

#include "nick.h"

namespace irc
{
	/** This class represents user connected to minbif */
	class User : public Nick
	{
		int fd;
		string password;
		time_t last_read;

	public:

		/** Build the User object.
		 *
		 * @param fd  file descriptor used to write messages to user.
		 * @param server  up-server (probably an IRC instance)
		 * @param nickname  nickname of user
		 * @param identname  identname of user
		 * @param hostname  hostname of user
		 * @param realname  realname of user
		 */
		User(int fd, Server* server, string nickname, string identname, string hostname, string realname="");
		~User();

		void setPassword(string p) { password = p; }
		string getPassword() const { return password; }

		void close() { fd = -1; }

		/** Set last read timestamp to now */
		void setLastReadNow();
		time_t getLastRead() const { return last_read; }

		/** Send a message to file descriptor */
		virtual void send(Message m);

	};

}; /* namespace irc */

#endif /* IRC_USER_H */
