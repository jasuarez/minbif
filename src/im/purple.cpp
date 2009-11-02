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

#include <libpurple/purple.h>
#include <cassert>

#include "purple.h"
#include "im.h"
#include "buddy.h"
#include "conversation.h"
#include "request.h"
#include "roomlist.h"
#include "ft.h"
#include "media.h"
#include "irc/irc.h"
#include "irc/buddy_icon.h"
#include "core/version.h"
#include "core/log.h"
#include "core/util.h"

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
		case PURPLE_DEBUG_FATAL:
			b_log[W_ERR] << "[" << category << "] " << args;
			break;
		case PURPLE_DEBUG_ERROR:
			b_log[W_PURPLE] << "[" << category << "] " << args;
			break;
		case PURPLE_DEBUG_WARNING:
			b_log[W_DEBUG] << "[" << category << "] " << args;
			break;
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

void Purple::minbif_prefs_init()
{
	purple_prefs_add_none("/minbif");

	purple_prefs_add_string("/minbif/password", "");
	purple_prefs_add_int("/minbif/typing_notice", 0);
}

GHashTable *Purple::ui_info = NULL;
GHashTable *Purple::minbif_ui_get_info(void)
{
	if (ui_info == NULL)
	{
		ui_info = g_hash_table_new(g_str_hash, g_str_equal);

		g_hash_table_insert(ui_info, (void*)"name",         (void*)MINBIF_VERSION_NAME);
		g_hash_table_insert(ui_info, (void*)"version",      (void*)MINBIF_VERSION);
		g_hash_table_insert(ui_info, (void*)"website",      (void*)MINBIF_WEBSITE);
		g_hash_table_insert(ui_info, (void*)"dev_website",  (void*)MINBIF_DEV_WEBSITE);
		g_hash_table_insert(ui_info, (void*)"client_type",  (void*)"pc");

	}

	return ui_info;
}


PurpleCoreUiOps Purple::core_ops =
{
	Purple::minbif_prefs_init,
        Purple::debug_init,
        Purple::inited,
        Purple::uninit,
        Purple::minbif_ui_get_info,

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
	if (!purple_core_init(MINBIF_VERSION_NAME))
	{
		b_log[W_ERR] << "Initialization of the Purple core failed.";
		throw PurpleError();
	}

	purple_set_blist(purple_blist_new());
	purple_blist_load();
}

void Purple::inited()
{
	Account::init();
	RoomList::init();
	Buddy::init();
	Conversation::init();
	Request::init();
	FileTransfert::init();
	Media::init();

	irc::IRC* irc = getIM()->getIRC();
	irc::BuddyIcon* bi = new irc::BuddyIcon(getIM(), irc);
	irc->addNick(bi);

	purple_prefs_set_bool("/purple/logging/log_ims", false);
	purple_prefs_set_bool("/purple/logging/log_chats", false);
	purple_prefs_set_bool("/purple/logging/log_system", false);
}

void Purple::uninit()
{
	assert(im != NULL);

	if(ui_info)
		g_hash_table_destroy(ui_info);
	Account::uninit();
	RoomList::uninit();
	Buddy::uninit();
	Conversation::uninit();
	Request::uninit();
	FileTransfert::uninit();
	Media::uninit();
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
	GList* list = purple_plugins_get_protocols();

	for(; list; list = list->next)
		if(id == ((PurplePlugin*)list->data)->info->id)
			return Protocol((PurplePlugin*)list->data);

	return Protocol();
}

string Purple::getNewAccountName(Protocol proto)
{
	GList* list = purple_accounts_get_all();
	GList* iter;
	int i = 0;

	for(iter = list; iter; iter = (iter ? iter->next : list))
	{
		Account acc((PurpleAccount*)iter->data);
		if(acc.getProtocol() != proto)
			continue;

		string id = acc.getID();
		if(id == proto.getID() + t2s(i))
		{
			i = s2t<int>(id.substr(proto.getID().size())) + 1;
			iter = NULL; // restart
		}
	}
	return proto.getID() + t2s(i);
}

Account Purple::addAccount(Protocol proto, string username, string password, vector<Protocol::Option> options)
{
	string id = getNewAccountName(proto);
	PurpleAccount *account = purple_account_new(username.c_str(), proto.getPurpleID().c_str());
	purple_accounts_add(account);
	purple_account_set_remember_password(account, TRUE);
	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "id", id.c_str());

	Account a(account);
	a.setID(id);
	a.setPassword(password);
	a.setOptions(options);

	const PurpleSavedStatus *saved_status;
	saved_status = purple_savedstatus_get_current();
	if (saved_status != NULL) {
		purple_savedstatus_activate_for_account(saved_status, account);
		purple_account_set_enabled(account, MINBIF_VERSION_NAME, TRUE);
	}

	return a;
}

void Purple::delAccount(PurpleAccount* account)
{
	purple_request_close_with_handle(account); /* Close any other opened delete window */
        purple_accounts_delete(account);
}

}; /* namespace im */
