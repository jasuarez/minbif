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

#include <purple.h>
#include <string>

namespace irc
{
	class ConvNick;
	class ConversationChannel;
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
		static void update_user(PurpleConversation* c, const char* user);
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

		/** Get a normalized IRC channel name.
		 *
		 * On IRC, channels name must begin with '#' and are uniq.
		 *
		 * For a given channel on a given account, the IRC channel name
		 * will be: "#name:accid".
		 *
		 * @param name  IM channel name
		 * @param acc  account which hosts this channel
		 * @return  an IRC channel name string.
		 */
		static string normalizeIRCName(string name, const Account& acc);

		string getChanName() const;
		string getChanTopic() const;
		Account getAccount() const;
		PurpleConversationType getType() const;

		/** Set the irc::Nick* object associated to this conversation.
		 *
		 * It is valid only if this is an IM conversation.
		 *
		 * @param n  irc::Nick object
		 * @param purge_unknown  if the current associated nick is an
		 *                       irc::UnknownBuddy object, purge it
		 *                       from IRC.
		 */
		void setNick(irc::ConvNick* n, bool purge_unknown = true);

		/** Get the irc::Nick* object associated to this conversation. */
		irc::ConvNick* getNick() const;

		void setChannel(irc::ConversationChannel* c) const;
		irc::ConversationChannel* getChannel() const;

		/** Create the IRC channel associated to this conversation. */
		void createChannel() const;

		/** Destroy the IRC channel associated to this conversation. */
		void destroyChannel() const;

		/** Present the conversation to user. */
		void present() const;

		/** Leave the conversation. */
		void leave();

		/** Send a command to this conversation. */
		int sendCommand(const string& cmd) const;

		/** Send a message in this conversation.
		 *
		 * @param text  text to send.
		 */
		void sendMessage(string text);

		/** A message is received, this function present it to user.
		 *
		 * @param from  nickname of sender
		 * @param text  text message
		 * @param action  this is an action (/me)
		 */
		void recvMessage(string from, string text, bool action = false);

		/** Invite a buddy into this conversation.
		 *
		 * This is available only for the CHAT conversations.
		 *
		 * @param buddy  buddy's name
		 * @param message  message to send with invitation.
		 */
		void invite(const string& buddy, const string& message);

		/** Set topic of conversation. */
		bool setTopic(const string& topic);
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
		string getAlias() const;
		string getRealName() const;
		bool isMe() const;
		int getChanStatus() const;

		Conversation getConversation() const { return conv; }
		Account getAccount() const;
		PurpleConvChatBuddy* getPurpleChatBuddy() const { return cbuddy; }

	};

}

#endif /* IM_CONVERSATION_H */
