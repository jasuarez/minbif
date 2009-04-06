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

#ifndef IRC_H
#define IRC_H

#include <string>
#include <exception>

#include "message.h"

using std::string;

class IRCAuthError : public std::exception {};

class Nick;
class _CallBack;

class IRC
{
	int fd;
	_CallBack *read_cb;
	string hostname;
	Nick* userNick;

public:

	IRC(int fd, string hostname="");
	~IRC();

	bool sendmsg(Message msg) const;

	string getServerName() const { return hostname; }

	Nick* getUserNick() const { return userNick; }

	void sendWelcome();

	void readIO(void*);
	void m_nick(Message m);
	void m_user(Message m);
};

#endif /* IRC_H */
