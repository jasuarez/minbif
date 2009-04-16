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
#include "purple_handler.h"
#include "im.h"
#include "../version.h"
#include "../log.h"

namespace im {

IM* Purple::im = NULL;

void Purple::init(IM* im)
{
	if(Purple::im)
	{
		b_log[W_ERR] << "These is already a purple instance!";
		throw PurpleError();
	}
	purple_util_set_user_dir(im->getUserPath().c_str());

	PurpleHandler::init();

	if (!purple_core_init(BITLBEE_VERSION_NAME))
	{
		b_log[W_ERR] << "Initialization of the Purple core failed.";
		throw PurpleError();
	}

	Purple::im = im;

	purple_set_blist(purple_blist_new());
	purple_blist_load();

	if (!purple_prefs_get_bool("/purple/savedstatus/startup_current_status"))
		        purple_savedstatus_activate(purple_savedstatus_get_startup());
	purple_accounts_restore_current_statuses();
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
		m[((PurplePlugin*)list->data)->info->id] = Protocol((PurplePlugin*)list->data);

	return m;
}

map<string, Account> Purple::getAccountsList()
{
	map<string, Account> m;
	GList* list = purple_accounts_get_all();

	for(; list; list = list->next)
		m[((PurpleAccount*)list->data)->protocol_id] = Account((PurpleAccount*)list->data);

	return m;
}
};
