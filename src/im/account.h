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

#ifndef IM_ACCOUNT_H
#define IM_ACCOUNT_H

#include <libpurple/purple.h>
#include <string>

#include "im/protocol.h"

namespace im
{
	using std::string;

	class Account
	{
		PurpleAccount* account;
		string id;
		Protocol proto;

		static void* getHandler();
		static void account_added(PurpleAccount*);

	public:

		static void init();

		Account();
		Account(PurpleAccount* account, string id, Protocol proto);

		string getID() const { return id; }
		string getUsername() const;
		Protocol getProtocol() const { return proto; }
	};

};

#endif /* IM_ACCOUNT_H */
