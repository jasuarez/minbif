/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon
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

#ifndef IM_PURPLE_H
#define IM_PURPLE_H

#include <map>
#include <string>

#include "plugin.h"
#include "protocol.h"
#include "account.h"
#include "core/log.h"

namespace im
{
	using std::map;
	using std::string;

	class IM;

	LOGEXCEPTION(PurpleError);

	/** Static class to interface with libpurple.
	 *
	 * Problem with libpurple is that it supports only one
	 * username per process, as there are ugly static variables
	 * to store states.
	 *
	 * This class is used to interface the POO-style of minbif
	 * with the poor C-static-style of libpurple.
	 */
	class Purple
	{
		/** Instanciation is forbidden */
		Purple() {}
		~Purple() {}

		static IM* im;

		static void inited();

		static GHashTable *ui_info;
		static PurpleEventLoopUiOps eventloop_ops;
		static PurpleCoreUiOps core_ops;
		static PurpleDebugUiOps debug_ops;

		static GHashTable *minbif_ui_get_info(void);
		static void minbif_prefs_init();

		static void debug_init();
		static void debug(PurpleDebugLevel level, const char *category, const char *args);

	public:

		/** Initialization */
		static void init(IM* im);

		/** Uninitialization */
		static void uninit();

		static IM* getIM() { return im; }

		static map<string, Plugin> getPluginsList();

		/** Get protocols list
		 *
		 * @return  map with first=id, second=Protocol object
		 */
		static map<string, Protocol> getProtocolsList();
		static Protocol getProtocolByPurpleID(string id);

		static map<string, Account> getAccountsList();
		static Account addAccount(const Protocol& proto, const string& username, const Protocol::Options& options, bool register_account);
		static void delAccount(PurpleAccount* account);
		static string getNewAccountName(Protocol proto, const Account& butone = Account());
	};
};

#endif /* IM_PURPLE_H */
