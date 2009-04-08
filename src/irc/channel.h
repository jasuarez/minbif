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

#ifndef CHANNEL_H
#define CHANNEL_H

#include <string>
using std::string;
#include <vector>
using std::vector;

#include "message.h"

class Nick;
class IRC;

class ChanUser
{
	Nick* nick;
	int status;

public:

	enum {
		OP     = 1 << 0,
		VOICE  = 1 << 1,
	};

	ChanUser(Nick* nick, int status = 0);

	bool hasStatus(int flag) const { return status & flag; }
	Nick* getNick() const { return nick; }
};

class Channel
{
	IRC* irc;
	string name;
	vector<ChanUser> users;

public:

	Channel(IRC* irc, string name);
	~Channel();

	static bool isChanName(string name)
	{
		if(name.empty()) return false;
		switch(name[0])
		{
			case '&':
			case '#':
				return true;
			default:
				return false;
		}
	}

	string getName() const { return name; }

	void addUser(Nick* nick, int status=0);

	void broadcast(Message m, Nick* butone = NULL);
};

#endif /* CHANNEL_H */
