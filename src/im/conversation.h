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

namespace irc
{
	class Nick;
}

namespace im
{
	using std::string;

	class Account;
	class Buddy;
	class ChatBuddy;

	class Conversation
	{
		PurpleConversation* conv;

		static void* getHandler();
		static PurpleConversationUiOps conv_ui_ops;

		static void create(PurpleConversation*);
		static void destroy(PurpleConversation*);
		static void conv_present(PurpleConversation*);
		static void write_im(PurpleConversation *conv, const char *who,
				const char *message, PurpleMessageFlags flags,
				time_t mtime);
		static void write_conv(PurpleConversation *conv, const char *who, const char* alias,
				const char *message, PurpleMessageFlags flags,
				time_t mtime);
		static void add_users(PurpleConversation *conv, GList *cbuddies,
				      gboolean new_arrivals);
		static void remove_user(PurpleConversation* conv, const char* cbname, const char *reason);
		static void topic_changed(PurpleConversation* conv, const char* who, const char* topic);
		static void buddy_typing(PurpleAccount* account, const char* who, gpointer null);
		static void chat_rename_user(PurpleConversation *conv, const char *old,
				             const char *new_n, const char *new_a);

	public:

		static void init();
		static void uninit();

		Conversation();
		Conversation(const Account& account, const Buddy& buddy);
		Conversation(const Account& account, const ChatBuddy& cbuddy);
		Conversation(const Account& account, const string& name);
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

		void setNick(irc::Nick* n, bool purge_unknown = true);
		irc::Nick* getNick() const;

		void createChannel() const;
		void destroyChannel() const;

		void present() const;
		void leave();
		void sendMessage(string text);
		void recvMessage(string from, string text, bool action = false);

		void invite(const string& buddy, const string& message);
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
		int getChanStatus() const;

		Conversation getConversation() const { return conv; }
		Account getAccount() const;
		PurpleConvChatBuddy* getPurpleChatBuddy() const { return cbuddy; }

	};

}

#endif /* IM_CONVERSATION_H */
