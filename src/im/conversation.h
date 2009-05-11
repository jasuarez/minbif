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

#ifndef IM_CONVERSATION_H
#define IM_CONVERSATION_H

#include <libpurple/purple.h>
#include <string>

namespace im
{
	using std::string;

	class Account;
	class Buddy;

	class Conversation
	{
		PurpleConversation* conv;

		static void* getHandler();
		static PurpleConversationUiOps conv_ui_ops;

		static void create(PurpleConversation*);
		static void destroy(PurpleConversation*);
		static void write_im(PurpleConversation *conv, const char *who,
				const char *message, PurpleMessageFlags flags,
				time_t mtime);
		static void write_conv(PurpleConversation *conv, const char *who, const char* alias,
				const char *message, PurpleMessageFlags flags,
				time_t mtime);
		static void add_users(PurpleConversation *conv, GList *cbuddies,
				      gboolean new_arrivals);
		static void topic_changed(PurpleConversation* conv, const char* who, const char* topic);

	public:

		static void init();

		Conversation();
		Conversation(Account account, Buddy buddy);
		Conversation(Account account, string name);
		Conversation(PurpleConversation* conv);

		bool operator==(const Conversation& conv) const;
		bool operator!=(const Conversation& conv) const;

		bool isValid() const { return conv != NULL; }
		PurpleConversation* getPurpleConversation() const { return conv; }
		PurpleConvChat* getPurpleChat() const;
		PurpleConvIm* getPurpleIm() const;

		string getName() const;
		string getChanName() const;
		string getChanTopic() const;
		Account getAccount() const;
		PurpleConversationType getType() const;

		void createChannel() const;
		void destroyChannel() const;

		void present() const;
		void leave();
		void sendMessage(string text) const;
		void recvMessage(string from, string text) const;
	};

	class ChatBuddy
	{
		Conversation conv;
		PurpleConvChatBuddy* cbuddy;

	public:

		ChatBuddy();
		ChatBuddy(Conversation conv, PurpleConvChatBuddy* cbuddy);

		bool operator==(const ChatBuddy& cbuddy) const;
		bool operator!=(const ChatBuddy& cbuddy) const;
		bool operator<(const ChatBuddy& cbuddy) const;

		bool isValid() const { return cbuddy != NULL; }

		string getName() const;
		string getRealName() const;
		bool isMe() const;

		Conversation getConversation() const { return conv; }
		Account getAccount() const;

	};

};

#endif /* IM_CONVERSATION_H */
