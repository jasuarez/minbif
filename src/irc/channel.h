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

#ifndef IRC_CHANNEL_H
#define IRC_CHANNEL_H

#include <string>
#include <vector>

#include "message.h"
#include "../entity.h"

namespace irc
{
	using std::vector;
	using std::string;


	class Nick;
	class IRC;
	class Channel;

	class ChanUser : public Entity
	{
		Nick* nick;
		Channel* chan;
		int status;

	public:

		enum {
			OP     = 1 << 0,
			VOICE  = 1 << 1,
		};

		ChanUser(Channel* chan, Nick* nick, int status = 0);

		string getName() const;

		bool hasStatus(int flag) const { return status & flag; }
		Nick* getNick() const { return nick; }

		Channel* getChannel() const { return chan; }
	};

	class Channel : public Entity
	{
		IRC* irc;
		vector<ChanUser> users;

	public:

		Channel(IRC* irc, string name);
		~Channel();

		static bool isChanName(const string& name)
		{
			return (!name.empty() && name.find(' ') == string::npos &&
				(isStatusChannel(name) || isRemoteChannel(name)));
		}
		static bool isStatusChannel(const string& name) { return (!name.empty() && name[0] == '&'); }
		static bool isRemoteChannel(const string& name) { return (!name.empty() && name[0] == '#'); }

		ChanUser addUser(Nick* nick, int status=0);
		void delUser(Nick* nick, Message message = Message());
		size_t countUsers() const { return users.size(); }

		void broadcast(Message m, Nick* butone = NULL);
	};
}; /*namespace irc*/

#endif /* IRC_CHANNEL_H */
