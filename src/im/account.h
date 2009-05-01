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

#ifndef IM_ACCOUNT_H
#define IM_ACCOUNT_H

#include <libpurple/purple.h>
#include <string>
#include <vector>

#include "im/protocol.h"

namespace im
{
	using std::string;
	using std::vector;
	class Buddy;

	class Account
	{
		PurpleAccount* account;
		Protocol proto;

		static PurpleConnectionUiOps conn_ops;
		static PurpleAccountUiOps acc_ops;
		static void* getHandler();
		static void account_added(PurpleAccount*);
		static void account_removed(PurpleAccount*);
		static void connecting(PurpleConnection *gc,
					const char *text,
					size_t step,
					size_t step_count);
		static void connected(PurpleConnection* gc);
		static void disconnected(PurpleConnection* gc);
		static void disconnect_reason(PurpleConnection *gc,
				PurpleConnectionError reason,
				const char *text);

	public:

		static void init();

		Account();
		Account(PurpleAccount* account, Protocol proto = Protocol());

		bool operator==(const Account&) const;
		bool operator!=(const Account&) const;

		bool isValid() const { return account != NULL; }
		PurpleAccount* getPurpleAccount() const { return account; }
		string getID() const;
		string getUsername() const;
		string getServername() const;
		string getStatusChannel() const;
		void setStatusChannel(string c);
		Protocol getProtocol() const { return proto; }
		bool isConnected() const;
		bool isConnecting() const;

		vector<Buddy> getBuddies() const;

		void connect() const;
		void disconnect() const;

		void createStatusChannel() const;
		void addBuddy(string username, string group) const;
		void removeBuddy(Buddy) const;
	};

};

#endif /* IM_ACCOUNT_H */
