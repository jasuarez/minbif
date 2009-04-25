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

#ifndef IM_IM_H
#define IM_IM_H

#include <exception>
#include <string>
#include <map>

#include "account.h"
#include "protocol.h"

namespace irc
{
	class IRC;
};

namespace im
{
	using std::string;
	using std::map;

	class IMError : public std::exception {};
	class ProtocolUnknown : public std::exception {};

	class IM
	{
		static string path;

	public:

		static void setPath(const string& path);
		static bool exists(const string& username);

	private:

		string username;
		string user_path;

		irc::IRC* irc;
	public:

		IM(irc::IRC* irc, string username);
		~IM();

		string getUserPath() const { return user_path; }

		void setPassword(const string& password);
		string getPassword() const;

		irc::IRC* getIRC() const { return irc; }

		map<string, Protocol> getProtocolsList() const;
		Protocol getProtocol(string id) const;

		map<string, Account> getAccountsList() const;
		Account getAccount(string name) const;
		Account getAccountFromChannel(string name) const;
		Account addAccount(Protocol proto, string username, string password);
		void delAccount(Account user);
	};

};

#endif /* IM_IM_H */
