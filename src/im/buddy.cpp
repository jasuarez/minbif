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
	serv_alias_buddy(buddy);
}

void Buddy::retrieveInfo() const
{
	assert(isValid() && buddy->account != NULL);
	serv_get_info(purple_account_get_connection(buddy->account), purple_buddy_get_name(buddy));
}

string Buddy::getRealName() const
{
	assert(isValid());
	const char* rn = purple_buddy_get_server_alias(buddy);
	if(rn)
		return rn;
	else
		return "";
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
	if(status)
		return purple_status_get_name(status);
	else
		return "bite";
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
		irc::Nick* n = Purple::getIM()->getIRC()->getNick(buddy);
		if(!n)
		{
			irc::Server* server = Purple::getIM()->getIRC()->getServer(buddy.getAccount().getServername());
			if(!server)
				return;

			n = new irc::Buddy(server, buddy);
			while(Purple::getIM()->getIRC()->getNick(n->getNickname()))
				n->setNickname(n->getNickname() + "_");

			Purple::getIM()->getIRC()->addNick(n);

			/* WARN! This function probably recalls this one, so it is
			 *       really NECESSARY to call them at end of this block.
			 *       If n is not in IRC's user list, two irc::Buddy
			 *       instances will be inserted, with one lost.
			 */
			buddy.setAlias(n->getNickname());
		}
		string channame = buddy.getAccount().getStatusChannel();
		irc::Channel* chan = Purple::getIM()->getIRC()->getChannel(channame);

		if(chan)
		{
			if(buddy.isOnline())
			{
				irc::ChanUser* chanuser = n->getChanUser(chan);
				if(!chanuser)
					n->join(chan, buddy.isAvailable() ? irc::ChanUser::VOICE : 0);
				else if(buddy.isAvailable() ^ chanuser->hasStatus(irc::ChanUser::VOICE))
				{
					if(buddy.isAvailable())
						chan->setMode(Purple::getIM()->getIRC(), irc::ChanUser::VOICE, chanuser);
					else
						chan->delMode(Purple::getIM()->getIRC(), irc::ChanUser::VOICE, chanuser);
				}

			}
			else if(!buddy.isOnline() && n->isOn(chan))
				n->quit("Leaving...");
		}

	}
}

void Buddy::removed_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		Buddy buddy = Buddy((PurpleBuddy*)node);
		irc::Nick* n = Purple::getIM()->getIRC()->getNick(buddy);
		if(n)
		{
			n->quit("Removed");
			Purple::getIM()->getIRC()->removeNick(n->getNickname());
		}
	}
}

}; /* namespace im */
