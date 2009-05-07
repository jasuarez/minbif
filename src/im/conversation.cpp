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

Conversation::Conversation(Account account, Buddy buddy)
	: conv(NULL)
{
	conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account.getPurpleAccount(), buddy.getName().c_str());
}

Conversation::Conversation(Account account, string name)
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

	string n = getName() + ":" + getAccount().getID();
	n = "#" + n;

	return n;
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
	purple_conversation_destroy(conv);
	conv = NULL;
}

void Conversation::createChannel() const
{
	assert(isValid());
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::ConversationChannel* chan = new irc::ConversationChannel(irc, *this);

	irc->addChannel(chan);

	irc->getUser()->join(chan);
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
	switch(getType())
	{
		case PURPLE_CONV_TYPE_IM:
			purple_conv_im_send(getPurpleIm(), text.c_str());
			break;
		case PURPLE_CONV_TYPE_CHAT:
			purple_conv_chat_send(getPurpleChat(), text.c_str());
			break;
		default:
			break;
	}
}

void Conversation::recvMessage(string from, string text) const
{
	irc::IRC* irc = Purple::getIM()->getIRC();
	switch(getType())
	{
		case PURPLE_CONV_TYPE_IM:
		{
			irc::Nick* n = irc->getNick(from);

			if(!n)
			{
				b_log[W_ERR] << "Received message from unknown budy " << from;
				return;
			}

			irc->getUser()->send(irc::Message(MSG_PRIVMSG).setSender(n)
								      .setReceiver(irc->getUser())
								      .addArg(text));
			break;
		}
		case PURPLE_CONV_TYPE_CHAT:
		{
			irc::ConversationChannel* chan = dynamic_cast<irc::ConversationChannel*>(irc->getChannel(getChanName()));
			if(!chan)
			{
				b_log[W_ERR] << "Received message in unknown chat " << getChanName();
				return;
			}

			irc::ChanUser* n = chan->getChanUser(from);
			if(!n)
			{
				b_log[W_ERR] << "Received message on " << getChanName() << " from an unknown budy " << from;
				return;
			}

			chan->broadcast(irc::Message(MSG_PRIVMSG).setSender(n)
					                         .setReceiver(chan)
								 .addArg(text));
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
	NULL,//Conversation::destroy,
	NULL,//finch_write_chat,
	Conversation::write_im,
	Conversation::write_conv,
	Conversation::add_users,
	NULL,//finch_chat_rename_user,
	NULL,//finch_chat_remove_users,
	NULL,//finch_chat_update_user,
	NULL,//finch_conv_present, /* present */
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

void Conversation::write_im(PurpleConversation *c, const char *who,
		const char *message, PurpleMessageFlags flags, time_t mtime)
{
	if(flags == PURPLE_MESSAGE_RECV)
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
	if(flags == PURPLE_MESSAGE_RECV)
	{
		Conversation conv = Conversation(c);

		char* newline = purple_strdup_withhtml(message);
		char* strip = purple_markup_strip_html(newline);

		string from;
		if(alias && *alias) from = alias;
		else if(who && *who) from = who;

		conv.recvMessage(from, strip);

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
		chan->addBuddy(cbuddy);
	}
}

}; /* namespace im */
