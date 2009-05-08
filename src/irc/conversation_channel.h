/*
 * Bitlbee v2 - IRC instant messaging gateway
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

#ifndef IRC_CONVERSATION_CHANNEL_H
#define IRC_CONVERSATION_CHANNEL_H

#include <map>

#include "channel.h"
#include "server.h"
#include "im/conversation.h"

namespace irc
{
	using std::map;

	class ConversationChannel : public Channel
	{
		im::Conversation conv;
		RemoteServer* upserver;

		map<im::ChatBuddy, ChanUser*> cbuddies;

	public:

		ConversationChannel(IRC* irc, const im::Conversation& conv);
		virtual ~ConversationChannel();

		virtual bool isStatusChannel() const { return false; }
		virtual bool isRemoteChannel() const { return true; }

		virtual void showBanList(Nick* to);

		virtual void processAddBan(Nick* from, string nick, string ident, string host, string accid);
		virtual void processRemoveBan(Nick* from, string nick, string ident, string host, string accid);

		virtual void broadcast(Message m, Nick* butone = NULL);
		virtual ChanUser* getChanUser(string nick) const;

		void addBuddy(im::ChatBuddy cbuddy);
		virtual void delUser(Nick* nick, Message message = Message());
	};
}; /* ns irc */

#endif /* IRC_CONVERSATION_CHANNEL_H */
