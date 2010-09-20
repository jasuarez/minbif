/*
 * Copyright(C) 2009-2010 Romain Bignon
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

#ifndef IRC_CONV_ENTITY_H
#define IRC_CONV_ENTITY_H

#include "im/conversation.h"
#include "irc/nick.h"

namespace irc
{
	class Server;

	class ConvEntity
	{
		im::Conversation conv;
		vector<string> enqueued_messages;

		bool flush_messages(void* data);

	public:
		ConvEntity(im::Conversation conv);

		/** Get the conversation associated to this nick */
		virtual im::Conversation getConversation() const { return conv; }

		/** Set the conversation associated. */
		virtual void setConversation(const im::Conversation& c) { conv = c; }

		/** Enqueue message to send to conversation. */
		virtual void enqueueMessage(const string& text, int delay);
	};

	class ConvNick : public Nick, public ConvEntity
	{
	public:

		ConvNick(Server* server, im::Conversation conv, string nickname,
		         string identname, string hostname, string realname = "");
		/** The ConvNick sends a message to someone. */
		virtual void sendMessage(Nick* to, const string& text, bool action = false);
	};

}; /* ns irc */

#endif /* IRC_CONV_ENTITY_H */
