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

#include "purple.h"
#include "../log.h"
#include "im/buddy.h"
#include "im/account.h"
#include "im/im.h"
#include "../util.h"
#include "irc/buddy.h"
#include "irc/irc.h"
#include "irc/channel.h"

namespace im {

Buddy::Buddy()
	: buddy(NULL)
{}

Buddy::Buddy(PurpleBuddy* _buddy)
	: buddy(_buddy)
{
}

bool Buddy::operator==(const Buddy& buddy) const
{
	return this->isValid() && buddy.isValid() && buddy.buddy == this->buddy;
}

bool Buddy::operator!=(const Buddy& buddy) const
{
	return !isValid() || !buddy.isValid() || buddy.buddy != this->buddy;
}

string Buddy::getName() const
{
	assert(isValid());
	const char* n = buddy->name;
	if(n)
		return n;
	else
		return "";
}

string Buddy::getAlias() const
{
	assert(isValid());
	const char* a = purple_buddy_get_alias(buddy);
	if(a && *a)
		return a;
	else
		return getName();
}

void Buddy::setAlias(string alias) const
{
	assert(isValid());
	purple_blist_alias_buddy(buddy, alias.c_str());

	/* XXX
	 * If this method is called while adding a new buddy in local
	 * blist, and if the email address doesn't exist, there is a race
	 * condition because the buddy isn't added on the MSN server,
	 * and the fucking libpurple's MSN plugin doesn't check it and crashes.
	 *
	 * It isn't possible to avoid sending alias to server, because at
	 * connection, local aliases are overrided by server aliases.
	 *
	 * So, for *now* (and until the pidgin crash will be fixed),
	 * I assume that minbif'll crash in this case, and I don't care
	 * about, you've had to correctly typing, fool!
	 *
	 * Ref: http://developer.pidgin.im/ticket/10393
	 */
	serv_alias_buddy(buddy);
}

irc::Buddy* Buddy::getNick() const
{
	assert(isValid());
	PurpleBlistNode* node = PURPLE_BLIST_NODE(buddy);
	return static_cast<irc::Buddy*>(node->ui_data);
}

void Buddy::setNick(irc::Buddy* b)
{
	assert(isValid());
	PurpleBlistNode* node = PURPLE_BLIST_NODE(buddy);
	node->ui_data = b;
}

void Buddy::retrieveInfo() const
{
	assert(isValid());
	assert(buddy->account != NULL);
	serv_get_info(purple_account_get_connection(buddy->account), purple_buddy_get_name(buddy));
}

void Buddy::sendFile(string filename)
{
	assert(isValid());
	serv_send_file(purple_account_get_connection(purple_buddy_get_account(buddy)),
		       purple_buddy_get_name(buddy),
		       filename.c_str());
}

void Buddy::updated() const
{
	string channame = getAccount().getStatusChannel();
	if(channame.empty())
		return;

	irc::Channel* chan = Purple::getIM()->getIRC()->getChannel(channame);

	if(!chan)
		return;

	irc::Buddy* n = getNick();
	if(isOnline())
	{
		irc::ChanUser* chanuser = n->getChanUser(chan);
		if(!chanuser)
			n->join(chan, isAvailable() ? irc::ChanUser::VOICE : 0);
		else if(isAvailable() ^ chanuser->hasStatus(irc::ChanUser::VOICE))
		{
			if(isAvailable())
				chan->setMode(Purple::getIM()->getIRC(), irc::ChanUser::VOICE, chanuser);
			else
				chan->delMode(Purple::getIM()->getIRC(), irc::ChanUser::VOICE, chanuser);
		}

	}
	else if(n->isOn(chan))
		n->quit("Signed-Off");
}

string Buddy::getRealName() const
{
	assert(isValid());
	const char* rn = purple_buddy_get_server_alias(buddy);
	if(rn && *rn)
		return rn;
	else
	{
		const char* servernick = purple_blist_node_get_string((PurpleBlistNode*)buddy, "servernick");
		if(servernick && *servernick)
			return servernick;
		else
			return getName();
	}
}

bool Buddy::isOnline() const
{
	assert(isValid());
	return PURPLE_BUDDY_IS_ONLINE(buddy);
}

bool Buddy::isAvailable() const
{
	assert(isValid());
	return purple_presence_is_available(purple_buddy_get_presence(buddy));
}

string Buddy::getStatus() const
{
	PurpleStatus* status = purple_presence_get_active_status(buddy->presence);
	if(!status)
		return "";

	string s = purple_status_get_name(status);

	PurplePlugin* prpl = purple_find_prpl(purple_account_get_protocol_id(buddy->account));
	PurplePluginProtocolInfo* prpl_info = NULL;

	if(prpl)
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

	if(prpl_info && prpl_info->status_text && buddy->account->gc)
	{
		char* tmp = prpl_info->status_text(buddy);
		const char* end;

		if(tmp)
		{
			if(!g_utf8_validate(tmp, -1, &end))
			{
				char* utf8 = g_strndup(tmp, g_utf8_pointer_to_offset(tmp, end));
				g_free(tmp);
				tmp = utf8;
			}
			char* tmp2 = purple_markup_strip_html(tmp);
			s += string(": ") + tmp2;
			g_free(tmp2);
			g_free(tmp);
		}
	}
	return s;
}

Account Buddy::getAccount() const
{
	assert(isValid());
	return Account(buddy->account);
}

CacaImage Buddy::getIcon() const
{
	assert(isValid());
	PurpleBuddyIcon* bicon = purple_buddy_icons_find(buddy->account, buddy->name);//purple_buddy_get_icon(buddy);
	if(!bicon)
		return CacaImage();

	return CacaImage(purple_buddy_icon_get_full_path(bicon));
}

string Buddy::getIconPath() const
{
	assert(isValid());
	PurpleBuddyIcon* bicon = purple_buddy_icons_find(buddy->account, buddy->name);//purple_buddy_get_icon(buddy);
	if(!bicon)
		return "";

	return purple_buddy_icon_get_full_path(bicon);
}

PurpleGroup* Buddy::getPurpleGroup() const
{
	assert(isValid());
	return purple_buddy_get_group(buddy);
}

/* STATIC */

PurpleBlistUiOps Buddy::blist_ui_ops =
{
        NULL,//new_list,
        NULL,//new_node,
        NULL,//blist_show,
        Buddy::update_node,//node_update,
        Buddy::removed_node,//node_remove,
        NULL,
        NULL,
        NULL,//finch_request_add_buddy,
        NULL,//finch_request_add_chat,
        NULL,//finch_request_add_group,
        NULL,
        NULL,
        NULL,
        NULL
};

void Buddy::init()
{
	purple_blist_set_ui_ops(&blist_ui_ops);
}

void Buddy::uninit()
{
	purple_blist_set_ui_ops(NULL);
}

void* Buddy::getHandler()
{
	static int handler;

	return &handler;
}

void Buddy::update_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		Buddy buddy = Buddy((PurpleBuddy*)node);
		irc::Buddy* n = buddy.getNick();
		if(!n)
		{
			irc::Server* server = Purple::getIM()->getIRC()->getServer(buddy.getAccount().getServername());
			if(!server)
				return;

			n = new irc::Buddy(server, buddy);
			while(Purple::getIM()->getIRC()->getNick(n->getNickname()))
				n->setNickname(n->getNickname() + "_");

			Purple::getIM()->getIRC()->addNick(n);
		}
		/* If server overrides the IRC nickname as alias, force it.
		 * WARN! This function probably recalls this one, so it is
		 *       really NECESSARY to call them at end of this block.
		 *       If n is not in IRC's user list, two irc::Buddy
		 *       instances will be inserted, with one lost.
		 */
		if(buddy.getAlias() != n->getNickname())
			buddy.setAlias(n->getNickname());

		buddy.updated();
	}
}

void Buddy::removed_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		Buddy buddy = Buddy((PurpleBuddy*)node);
		irc::Buddy* n = buddy.getNick();
		if(!n)
			n = dynamic_cast<irc::Buddy*>(Purple::getIM()->getIRC()->getNick(buddy));
		if(n)
		{
			n->quit("Removed");
			Purple::getIM()->getIRC()->removeNick(n->getNickname());
		}
	}
}

}; /* namespace im */
