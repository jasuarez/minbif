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

#include "../log.h"
#include "im/buddy.h"

namespace im {

Buddy::Buddy()
	: buddy(NULL)
{}

Buddy::Buddy(PurpleBuddy* _buddy, const Account& _account)
	: buddy(_buddy),
	  account(_account)
{}

string Buddy::getName() const
{
	assert(isValid());
	return buddy->name;
}

/* STATIC */

PurpleBlistUiOps Buddy::blist_ui_ops =
{
        NULL,//new_list,
        NULL,//new_node,
        NULL,//blist_show,
        Buddy::update_node,//node_update,
        NULL,//node_remove,
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

void* Buddy::getHandler()
{
	static int handler;

	return &handler;
}

void Buddy::update_node(PurpleBuddyList *list, PurpleBlistNode *node)
{
	if (PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		Buddy buddy = Buddy((PurpleBuddy*)node, Account(((PurpleBuddy*)node)->account));
		b_log[W_ERR] << buddy.getName() << "   " << (PURPLE_BUDDY_IS_ONLINE((PurpleBuddy*)node) ? "online" : "offline");
	}
}

}; /* namespace im */
