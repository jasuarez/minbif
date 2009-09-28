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

#ifndef IRC_CHAT_BUDDY_H
#define IRC_CHAT_BUDDY_H

#include "nick.h"
#include "im/conversation.h"

namespace irc
{

	/** This class represents a chat buddy on IRC */
	class ChatBuddy : public Nick
	{
		im::ChatBuddy im_cbuddy;
		im::Conversation conv;

	public:

		/** Build buddy object
		 *
		 * @param server  up-server
		 * @param cbuddy  IM buddy object.
		 */
		ChatBuddy(Server* server, im::ChatBuddy cbuddy);
		~ChatBuddy();

		/** Implementation of the message routing to this buddy */
		virtual void send(Message m);

		/** Get buddy's away message. */
		virtual string getAwayMessage() const;

		/** Check if user is away or not. */
		virtual bool isAway() const;

		/** Check if buddy is online or not. */
		virtual bool isOnline() const { return true; }

		im::ChatBuddy getChatBuddy() const { return im_cbuddy; }
		void setChatBuddy(im::ChatBuddy cb) { im_cbuddy = cb; }

		/** Get buddy's real name. */
		virtual string getRealName() const;

	};

}; /* namespace irc */

#endif /* IRC_CHAT_BUDDY_H */
