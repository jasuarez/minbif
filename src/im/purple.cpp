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

#include <libpurple/purple.h>
#include <cassert>

#include "purple.h"
#include "im.h"
#include "buddy.h"
#include "conversation.h"
#include "request.h"
#include "../version.h"
#include "../log.h"
#include "../util.h"

namespace im {

IM* Purple::im = NULL;

PurpleEventLoopUiOps Purple::eventloop_ops =
{
	/* timeout_add */    g_timeout_add,
	/* timeout_remove */ g_source_remove,
	/* input_add */      glib_input_add,
	/* input_remove */   g_source_remove,
	/* input_get_error*/ NULL,
	/* timeout_add_seconds */ NULL,
	NULL, NULL, NULL
};

void Purple::debug(PurpleDebugLevel level, const char *category, const char *args)
{
	switch(level)
	{
		case PURPLE_DEBUG_WARNING:
		case PURPLE_DEBUG_ERROR:
		case PURPLE_DEBUG_FATAL:
			b_log[W_WARNING] << "[" << category << "] " << args;
		default:
			break;
	}
}

PurpleDebugUiOps Purple::debug_ops =
{
        Purple::debug,
        NULL, //finch_debug_is_enabled,

        /* padding */
        NULL,
        NULL,
        NULL,
        NULL
};

void Purple::debug_init()
{
	purple_debug_set_ui_ops(&debug_ops);
}

void Purple::bitlbee_prefs_init()
{
	purple_prefs_add_none("/bitlbee");

	purple_prefs_add_string("/bitlbee/password", "");
}

GHashTable *Purple::ui_info = NULL;
GHashTable *Purple::bitlbee_ui_get_info(void)
{
        if (ui_info == NULL) {
                ui_info = g_hash_table_new(g_str_hash, g_str_equal);

                g_hash_table_insert(ui_info, (void*)"name",         (void*)BITLBEE_VERSION_NAME);
                g_hash_table_insert(ui_info, (void*)"version",      (void*)BITLBEE_VERSION);
                g_hash_table_insert(ui_info, (void*)"website",      (void*)"http://symlink.me/wiki/bitlbee");
                g_hash_table_insert(ui_info, (void*)"dev_website",  (void*)"http://symlink.me/projects/show/bitlbee2");
        }

        return ui_info;
}


PurpleCoreUiOps Purple::core_ops =
{
	Purple::bitlbee_prefs_init,
        Purple::debug_init,
        Purple::inited,//gnt_ui_init,
        NULL,//bitlbee_quit,
        Purple::bitlbee_ui_get_info,

        /* padding */
        NULL,
        NULL,
        NULL
};

void Purple::init(IM* im)
{
	if(Purple::im)
	{
		b_log[W_ERR] << "These is already a purple instance!";
		throw PurpleError();
	}
	purple_util_set_user_dir(im->getUserPath().c_str());

	purple_core_set_ui_ops(&core_ops);
	purple_eventloop_set_ui_ops(&eventloop_ops);

	Purple::im = im;
	if (!purple_core_init(BITLBEE_VERSION_NAME))
	{
		b_log[W_ERR] << "Initialization of the Purple core failed.";
		throw PurpleError();
	}

	purple_set_blist(purple_blist_new());
	purple_blist_load();

	if (!purple_prefs_get_bool("/purple/savedstatus/startup_current_status"))
		purple_savedstatus_activate(purple_savedstatus_get_startup());
	purple_accounts_restore_current_statuses();
}

void Purple::inited()
{
	Account::init();
	Buddy::init();
	Conversation::init();
	Request::init();
}

void Purple::uninit()
{
	assert(im != NULL);

	purple_core_quit();
}

map<string, Protocol> Purple::getProtocolsList()
{
	map<string, Protocol> m;
	GList* list = purple_plugins_get_protocols();

	for(; list; list = list->next)
	{
		Protocol protocol = Protocol((PurplePlugin*)list->data);
		m[protocol.getID()] = protocol;
	}

	return m;
}

map<string, Account> Purple::getAccountsList()
{
	map<string, Account> m;
	GList* list = purple_accounts_get_all();
	map<string, Protocol> protocols = getProtocolsList();

	for(; list; list = list->next)
	{
		Protocol proto = getProtocolByPurpleID(((PurpleAccount*)list->data)->protocol_id);

		Account account = Account((PurpleAccount*)list->data, proto);
		m[account.getID()] = account;
	}

	return m;
}

Protocol Purple::getProtocolByPurpleID(string id)
{
	map<string, Protocol> protocols = getProtocolsList();

	for(map<string, Protocol>::const_iterator it = protocols.begin(); it != protocols.end(); ++it)
		if(it->second.getPurpleID() == id)
			return it->second;
	return Protocol();
}

string Purple::getNewAccountName(Protocol proto)
{
	GList* list = purple_accounts_get_all();
	int i = 0;

	for(; list; list = list->next)
	{
		Protocol account_proto = getProtocolByPurpleID(((PurpleAccount*)list->data)->protocol_id);
		if(account_proto != proto)
			continue;

		string id = purple_account_get_ui_string(((PurpleAccount*)list->data), BITLBEE_VERSION_NAME, "id", "");
		if(id == proto.getID() + t2s(i))
			i = s2t<int>(id.substr(proto.getID().size())) + 1;
	}
	return proto.getID() + t2s(i);
}

Account Purple::addAccount(Protocol proto, string username, string password)
{
	string id = getNewAccountName(proto);
	PurpleAccount *account = purple_account_new(username.c_str(), proto.getPurpleID().c_str());

	purple_accounts_add(account);
	purple_account_set_remember_password(account, TRUE);
	purple_account_set_password(account, password.c_str());
	purple_account_set_ui_string(account, BITLBEE_VERSION_NAME, "id", id.c_str());

	const PurpleSavedStatus *saved_status;
	saved_status = purple_savedstatus_get_current();
	if (saved_status != NULL) {
		purple_savedstatus_activate_for_account(saved_status, account);
		purple_account_set_enabled(account, BITLBEE_VERSION_NAME, TRUE);
	}

	return Account(account, proto);
}

void Purple::delAccount(PurpleAccount* user)
{
	purple_request_close_with_handle(user); /* Close any other opened delete window */
        purple_accounts_delete(user);
}

}; /* namespace im */
