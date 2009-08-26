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

#include <cassert>
#include <cstring>

#include "im/conversation.h"
#include "im/purple.h"
#include "im/buddy.h"
#include "im/im.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "irc/message.h"
#include "irc/conversation_channel.h"
#include "irc/chat_buddy.h"
#include "../log.h"

namespace im {

/*****
 * ChatBuddy
 */

ChatBuddy::ChatBuddy(Conversation _conv, PurpleConvChatBuddy* _cbuddy)
	: conv(_conv), cbuddy(_cbuddy)
{}

ChatBuddy::ChatBuddy()
	: cbuddy(NULL)
{}

string ChatBuddy::getName() const
{
	assert(isValid());
	return cbuddy->name;
}

string ChatBuddy::getRealName() const
{
	assert(isValid());

	PurplePluginProtocolInfo* prpl = conv.getAccount().getProtocol().getPurpleProtocol();
	if(prpl->get_cb_real_name)
		return prpl->get_cb_real_name(conv.getAccount().getPurpleConnection(), conv.getPurpleChat()->id, cbuddy->name);
	else
		return getName();
}

int ChatBuddy::getChanStatus() const
{
	assert(isValid());

	return cbuddy->flags;
}

Account ChatBuddy::getAccount() const
{
	assert(isValid());
	return conv.getAccount();
}

bool ChatBuddy::isMe() const
{
	assert(isValid());
	return getName() == purple_conv_chat_get_nick(conv.getPurpleChat());
}

bool ChatBuddy::operator==(const ChatBuddy& cbuddy) const
{
	return this->cbuddy == cbuddy.cbuddy;
}

bool ChatBuddy::operator!=(const ChatBuddy& cbuddy) const
{
	return this->cbuddy != cbuddy.cbuddy;
}

bool ChatBuddy::operator<(const ChatBuddy& cbuddy) const
{
	return this->cbuddy < cbuddy.cbuddy;
}

/****
 * Conversation
 */
Conversation::Conversation()
	: conv(NULL)
{}

Conversation::Conversation(PurpleConversation* _conv)
	: conv(_conv)
{}

Conversation::Conversation(const Account& account, const Buddy& buddy)
	: conv(NULL)
{
	conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account.getPurpleAccount(), buddy.getName().c_str());
}

Conversation::Conversation(const Account& account, const ChatBuddy& cbuddy)
	: conv(NULL)
{
	conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account.getPurpleAccount(), cbuddy.getRealName().c_str());
}

Conversation::Conversation(const Account& account, const string& name)
	: conv(NULL)
{
	conv = purple_conversation_new(PURPLE_CONV_TYPE_CHAT, account.getPurpleAccount(), name.c_str());
}

bool Conversation::operator==(const Conversation& conv) const
{
	return conv.conv == this->conv;
}

bool Conversation::operator!=(const Conversation& conv) const
{
	return conv.conv != this->conv;
}

Account Conversation::getAccount() const
{
	assert(isValid());

	return Account(conv->account);
}

string Conversation::getName() const
{
	assert(isValid());

	const char* name = conv->name;
	if(name)
		return name;
	else
		return "";
}

string Conversation::getChanName() const
{
	assert(isValid());

	string n = "#" + getName() + ":" + getAccount().getID();

	return n;
}

string Conversation::getChanTopic() const
{
	assert(isValid());

	const char* topic = purple_conv_chat_get_topic(getPurpleChat());
	if(topic)
		return topic;
	else
		return "";
}

PurpleConversationType Conversation::getType() const
{
	assert(isValid());
	return purple_conversation_get_type(conv);
}

void Conversation::present() const
{
	assert(isValid());
	purple_conversation_present(conv);
}

void Conversation::leave()
{
	assert(isValid());

	/* As this method can be called by ConversationChannel, which owns me,
	 * it isn't a good idea to change myself after call of
	 * purple_conversation_destroy().
	 */
	PurpleConversation* c = conv;
	conv = NULL;
	purple_conversation_destroy(c);
}

void Conversation::invite(const string& buddy, const string& message)
{
	assert(isValid());

	serv_chat_invite(purple_conversation_get_gc(conv),
			        purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv)),
				message.c_str(), buddy.c_str());
}

void Conversation::createChannel() const
{
	assert(isValid());
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::ConversationChannel* chan = new irc::ConversationChannel(irc, *this);

	irc->addChannel(chan);

	/* XXX As callback add_users will be called and I'll be in added users,
	 *     a better idea could be do not join here, but wait for the add_users
	 *     where i'll be.
	 *     BUT, there is a bug with the jabber plugin, so it is possible this
	 *     event will never happen.
	 */
	irc->getUser()->join(chan);
	GList* l = purple_conv_chat_get_users(getPurpleChat());
	for (; l != NULL; l = l->next)
	{
		ChatBuddy cbuddy = ChatBuddy(conv, (PurpleConvChatBuddy *)l->data);
		b_log[W_ERR] << cbuddy.getName();
	}
}

void Conversation::destroyChannel() const
{
	assert(isValid());
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc->removeChannel(getChanName());
}

PurpleConvChat* Conversation::getPurpleChat() const
{
	assert(isValid());
	return PURPLE_CONV_CHAT(conv);
}

PurpleConvIm* Conversation::getPurpleIm() const
{
	assert(isValid());
	return PURPLE_CONV_IM(conv);
}

void Conversation::sendMessage(string text) const
{
	assert(isValid());

	if(text.find("\001ACTION ") == 0)
	{
		/* Check if this protocol has registered the /me command. */
		text = "/me " + text.substr(8, text.size() - 8 - 1);
		char *escape = g_markup_escape_text(text.c_str() + 1, -1);
		char* error = NULL;
		int status = purple_cmd_do_command(conv, text.c_str() + 1, escape, &error);

		g_free(error);
		g_free(escape);

		if(status == PURPLE_CMD_STATUS_OK)
			return;

		/* If the /me command is not implemented for this protocol, just
		 * continue to send message prefixed with /me. */
	}

	if(text.find("\001TYPING ") == 0)
	{
		string typing = text.substr(8, text.size() - 8 - 1);
		int timeout;
		if (typing == "1")
		{
				timeout = serv_send_typing(purple_conversation_get_gc(conv),
							   purple_conversation_get_name(conv),
							   PURPLE_TYPING);
				purple_conv_im_set_type_again(getPurpleIm(), timeout);
		}
		else if (typing == "0")
		{
				purple_conv_im_stop_send_typed_timeout(getPurpleIm());
				timeout = serv_send_typing(purple_conversation_get_gc(conv),
							   purple_conversation_get_name(conv),
							   PURPLE_NOT_TYPING);
		}
		return;
	}

	char *escape = g_markup_escape_text(text.c_str(), -1);
	char *apos = purple_strreplace(escape, "&apos;", "'");
	g_free(escape);
	escape = apos;

	switch(getType())
	{
		case PURPLE_CONV_TYPE_IM:
			purple_conv_im_send_with_flags(getPurpleIm(), escape, PURPLE_MESSAGE_SEND);
			break;
		case PURPLE_CONV_TYPE_CHAT:
			purple_conv_chat_send(getPurpleChat(), escape);
			break;
		default:
			break;
	}
	g_free(escape);
	purple_idle_touch();
}

void Conversation::recvMessage(string from, string text, bool action) const
{
	assert(isValid());
	irc::IRC* irc = Purple::getIM()->getIRC();
	switch(getType())
	{
		case PURPLE_CONV_TYPE_IM:
		{
			irc::Nick* n = irc->getNick(from);

			if(!n)
			{
				b_log[W_ERR] << "Received message from unknown budy " << from << ": " << text;
				return;
			}

			string line;
			while((line = stringtok(text, "\n\r")).empty() == false)
			{
				if(action)
					line = "\001ACTION " + line + "\001";
				irc->getUser()->send(irc::Message(MSG_PRIVMSG).setSender(n)
									      .setReceiver(irc->getUser())
									      .addArg(line));
			}
			break;
		}
		case PURPLE_CONV_TYPE_CHAT:
		{
			irc::ConversationChannel* chan = dynamic_cast<irc::ConversationChannel*>(irc->getChannel(getChanName()));
			if(!chan)
			{
				b_log[W_ERR] << "Received message in unknown chat " << getChanName() << ": " << text;
				return;
			}

			irc::ChanUser* n = chan->getChanUser(from);
			if(!n)
			{
				while((irc->getNick(from)))
					from += "_";
			}

			string line;
			while((line = stringtok(text, "\n\r")).empty() == false)
			{
				if(action)
					line = "\001ACTION " + line + "\001";
				if(n)
					chan->broadcast(irc::Message(MSG_PRIVMSG).setSender(n)
										 .setReceiver(chan)
										 .addArg(line));
				else
					chan->broadcast(irc::Message(MSG_PRIVMSG).setSender(from)
										 .setReceiver(chan)
										 .addArg(line));
			}
			break;
		}
		default:
			break;
	}
}

/* STATIC */

PurpleConversationUiOps Conversation::conv_ui_ops =
{
	Conversation::create,
	NULL,//finch_destroy (use signal instead)
	NULL,//finch_write_chat,
	Conversation::write_im,
	Conversation::write_conv,
	Conversation::add_users,
	Conversation::chat_rename_user,
	NULL,//Conversation::remove_users, use signal instead
	NULL,//finch_chat_update_user,
	Conversation::conv_present,//finch_conv_present, /* present */
	NULL,//finch_conv_has_focus, /* has_focus */
	NULL, /* custom_smiley_add */
	NULL, /* custom_smiley_write */
	NULL, /* custom_smiley_close */
	NULL, /* send_confirm */
	NULL,
	NULL,
	NULL,
	NULL
};

void Conversation::init()
{
	purple_conversations_set_ui_ops(&conv_ui_ops);
	purple_signal_connect(purple_conversations_get_handle(), "deleting-conversation",
				getHandler(), PURPLE_CALLBACK(destroy),
				NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-topic-changed",
			        getHandler(), PURPLE_CALLBACK(topic_changed),
				NULL);
	purple_signal_connect(purple_conversations_get_handle(), "chat-buddy-leaving",
				getHandler(), PURPLE_CALLBACK(remove_user),
				NULL);
	purple_signal_connect(purple_conversations_get_handle(), "buddy-typing",
			        getHandler(), PURPLE_CALLBACK(buddy_typing),
				NULL);
	purple_signal_connect(purple_conversations_get_handle(), "buddy-typing-stopped",
			        getHandler(), PURPLE_CALLBACK(buddy_typing),
				NULL);
}

void Conversation::uninit()
{
	purple_conversations_set_ui_ops(NULL);
	purple_signals_disconnect_by_handle(getHandler());
}

void* Conversation::getHandler()
{
	static int handler;

	return &handler;
}

void Conversation::create(PurpleConversation* c)
{
	Conversation conv(c);

	switch(conv.getType())
	{
		case PURPLE_CONV_TYPE_IM:
			break;
		case PURPLE_CONV_TYPE_CHAT:
			conv.createChannel();
			break;
		default:
			break;
	}
}

void Conversation::destroy(PurpleConversation* c)
{
	Conversation conv(c);

	switch(conv.getType())
	{
		case PURPLE_CONV_TYPE_IM:
			break;
		case PURPLE_CONV_TYPE_CHAT:
			conv.destroyChannel();
			break;
		default:
			break;
	}
}

void Conversation::conv_present(PurpleConversation* c)
{
}

void Conversation::write_im(PurpleConversation *c, const char *who,
		const char *message, PurpleMessageFlags flags, time_t mtime)
{
	if(flags & PURPLE_MESSAGE_RECV)
	{
		PurpleAccount *account = purple_conversation_get_account(c);
		PurpleBuddy *buddy;
		who = purple_conversation_get_name(c);
		buddy = purple_find_buddy(account, who);
		if (buddy)
			who = purple_buddy_get_contact_alias(buddy);
	}

	purple_conversation_write(c, who, message, flags, mtime);
}

void Conversation::write_conv(PurpleConversation *c, const char *who, const char* alias,
		const char *message, PurpleMessageFlags flags, time_t mtime)
{
	if ((flags & PURPLE_MESSAGE_SYSTEM) && !(flags & PURPLE_MESSAGE_NOTIFY))
		flags = (PurpleMessageFlags)(flags & ~(PURPLE_MESSAGE_SEND | PURPLE_MESSAGE_RECV));

	if(flags & PURPLE_MESSAGE_RECV)
	{
		Conversation conv = Conversation(c);

		bool action = false;
		if(who && purple_message_meify((char*)message, -1))
				action = true;

		char* newline = purple_strdup_withhtml(message);
		char* strip = purple_markup_strip_html(newline);
		string from;

		if(alias && *alias) from = alias;
		else if(who && *who) from = who;

		if(flags & PURPLE_MESSAGE_DELAYED)
		{
			struct tm* lt = localtime(&mtime);
			char* msg;
			msg = g_strdup_printf("[%02d:%02d:%02d] %s", lt->tm_hour,
					                             lt->tm_min,
							             lt->tm_sec,
							             strip);
			conv.recvMessage(from, msg, action);
			g_free(msg);
		}
		else
			conv.recvMessage(from, strip ? strip : "", action);

		g_free(strip);
		g_free(newline);
	}
}

void Conversation::add_users(PurpleConversation *c, GList *cbuddies,
			     gboolean new_arrivals)
{
	Conversation conv(c);
	GList* l = cbuddies;
	for (; l != NULL; l = l->next)
	{
		ChatBuddy cbuddy = ChatBuddy(conv, (PurpleConvChatBuddy *)l->data);

		irc::IRC* irc = Purple::getIM()->getIRC();
		irc::ConversationChannel* chan = dynamic_cast<irc::ConversationChannel*>(irc->getChannel(conv.getChanName()));
		if(!chan)
		{
			b_log[W_ERR] << "Conversation channel doesn't exist: " << conv.getChanName();
			return;
		}
		chan->addBuddy(cbuddy, cbuddy.getChanStatus());
	}
}

void Conversation::remove_user(PurpleConversation* c, const char* cbname, const char *reason)
{
	Conversation conv(c);
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::ConversationChannel* chan = dynamic_cast<irc::ConversationChannel*>(irc->getChannel(conv.getChanName()));
	if(!chan)
	{
		b_log[W_ERR] << "Conversation channel doesn't exist: " << conv.getChanName();
		return;
	}
	irc::Nick* nick = chan->getChanUser(cbname)->getNick();
	nick->part(chan, reason ? reason : "");
}

void Conversation::chat_rename_user(PurpleConversation *c, const char *old,
				    const char *new_n, const char *new_a)
{
	Conversation conv(conv);
	ChatBuddy cbuddy(conv, purple_conv_chat_cb_find(conv.getPurpleChat(), new_n));
	if(!cbuddy.isValid())
	{
		b_log[W_ERR] << "Rename from " << old << " to " << new_n << " (" << new_a << ") which is an unknown chat buddy";
		return;
	}
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::ConversationChannel* chan = dynamic_cast<irc::ConversationChannel*>(irc->getChannel(conv.getChanName()));
	if(!chan)
	{
		b_log[W_ERR] << "Conversation channel doesn't exist: " << conv.getChanName();
		return;
	}
	irc::Nick* nick = chan->getChanUser(old)->getNick();
	string new_nick = new_a;
	while(irc->getNick(new_nick))
		new_nick += "_";

	irc->getUser()->send(irc::Message(MSG_NICK).setSender(nick)
			                           .addArg(new_nick));
	nick->setNickname(new_nick);
}

void Conversation::topic_changed(PurpleConversation* c, const char* who, const char* topic)
{
	Conversation conv(c);
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::ConversationChannel* chan = dynamic_cast<irc::ConversationChannel*>(irc->getChannel(conv.getChanName()));
	if(!chan)
	{
		b_log[W_ERR] << "Conversation channel doesn't exist: " << conv.getChanName();
		return;
	}
	irc::ChanUser* chanuser = 0;
	if(who)
		chanuser = chan->getChanUser(who);
	chan->setTopic(chanuser, topic ? topic : "");
}

void Conversation::buddy_typing(PurpleAccount* account, const char* who, gpointer null)
{
	Conversation conv(purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account));
	irc::IRC* irc = Purple::getIM()->getIRC();
	PurpleConvIm *im = NULL;

	if(!conv.isValid() || who == NULL)
		return;

	im = conv.getPurpleIm();

	PurpleBuddy *buddy = purple_find_buddy(account, who);
	irc::Nick* n = irc->getNick(buddy);

	if(!n)
		return;

	if(purple_conv_im_get_typing_state(im) == PURPLE_TYPING)
		irc->getUser()->send(irc::Message(MSG_PRIVMSG).setSender(n)
							 .setReceiver(irc->getUser())
							 .addArg("\1TYPING 1\1"));
	else
		irc->getUser()->send(irc::Message(MSG_PRIVMSG).setSender(n)
							 .setReceiver(irc->getUser())
				                         .addArg("\1TYPING 0\1"));
}

}; /* namespace im */
