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

#include <cassert>

#include "im/conversation.h"
#include "im/purple.h"
#include "im/buddy.h"
#include "im/im.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "irc/message.h"
#include "../log.h"

namespace im {

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

bool Conversation::operator==(const Conversation& conv) const
{
	return conv.conv == this->conv;
}

bool Conversation::operator!=(const Conversation& conv) const
{
	return conv.conv != this->conv;
}

void Conversation::present() const
{
	assert(isValid());
	purple_conversation_present(conv);
}

void Conversation::sendMessage(string text) const
{
	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM)
			purple_conv_im_send(PURPLE_CONV_IM(conv), text.c_str());
	else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT)
			purple_conv_chat_send(PURPLE_CONV_CHAT(conv), text.c_str());
}

/* STATIC */

PurpleConversationUiOps Conversation::conv_ui_ops =
{
	Conversation::create,//finch_create_conversation,
	NULL,//finch_destroy_conversation,
	NULL,//finch_write_chat,
	Conversation::write_im,//finch_write_im,
	NULL,//finch_write_conv,
	NULL,//finch_chat_add_users,
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
}

void* Conversation::getHandler()
{
	static int handler;

	return &handler;
}

void Conversation::create(PurpleConversation* c)
{
}

void Conversation::write_im(PurpleConversation *conv, const char *who,
		const char *message, PurpleMessageFlags flags,
		time_t mtime)
{
	if(flags == PURPLE_MESSAGE_RECV && who)
	{
		char* newline = purple_strdup_withhtml(message);
		char* strip = purple_markup_strip_html(newline);

		Purple::getIM()->getIRC()->getUser()->send(irc::Message(MSG_PRIVMSG).setSender(who)
									       .setReceiver(Purple::getIM()->getIRC()->getUser())
									       .addArg(strip));

		g_free(strip);
		g_free(newline);
	}
}

}; /* namespace im */
