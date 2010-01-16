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

/** IM related classes */
namespace im
{
	using std::string;
	using std::map;

	/** Raised when IM can't be initialized. */
	class IMError : public std::exception {};

	/** Protocol is unknown */
	class ProtocolUnknown : public std::exception {};

	/** Class used to Instant Messaging things */
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

		/** Constructor.
		 *
		 * @param irc  pointer to the IRC instance
		 * @param username user name
		 */
		IM(irc::IRC* irc, string username);
		~IM();

		/** Restore previous status */
		void restore();

		/** Get path to user settings */
		string getUserPath() const { return user_path; }

		/** Get username of this account. */
		string getUsername() const { return username; }

		/** Set user password */
		void setPassword(const string& password);
		string getPassword() const;

		/** Set typing notice */
		void setTypingNotice(bool enabled);
		bool hasTypingNotice() const;

		/** Set away idle */
		void setAwayIdle(bool enabled);
		bool hasAwayIdle() const;

		/** Set path to the buddy icon.
		 *
		 * This buddy icon will used on every IM accounts.
		 */
		void setBuddyIcon(string path);

		/** Get path to current buddy icon. */
		string getBuddyIconPath() const;

		irc::IRC* getIRC() const { return irc; }

		/** Get list of protocols in a map. */
		map<string, Protocol> getProtocolsList() const;

		/** Get a protocol from id
		 *
		 * @param id  protocol's id
		 * @return  a Protocol instance
		 */
		Protocol getProtocol(string id) const;

		/** Get list of accounts in a map. */
		map<string, Account> getAccountsList() const;

		/** Get an account from name */
		Account getAccount(string name) const;

		/** Get an account from status channel name. */
		Account getAccountFromChannel(string name) const;

		/** Create an account.
		 *
		 * @param proto  protocol used by this account
		 * @param username  username of this account
		 * @param password  password of this account
		 * @param options  list of specific options of this protocol
		 * @param register_account  register this account on server
		 * @return  an Account instance.
		 */
		Account addAccount(Protocol proto, string username, string password, vector<Protocol::Option> options, bool register_account = false);

		/** Remove an account.
		 *
		 * @param account  Account instance
		 */
		void delAccount(Account account);

		/** Set status on IM accounts */
		bool setStatus(string status);
	};

};

#endif /* IM_IM_H */
