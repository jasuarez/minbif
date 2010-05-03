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

#ifndef IRC_STATUS_CHANNEL_H
#define IRC_STATUS_CHANNEL_H

#include <vector>

#include "channel.h"
#include "im/account.h"

namespace irc
{
	using std::vector;

	class StatusChannel : public Channel
	{
		vector<im::Account> accounts;

		string getMaskFromName(const string& name, const im::Account& acc) const;

	public:
		StatusChannel(IRC* irc, string name);
		virtual ~StatusChannel() {}

		virtual bool isStatusChannel() const { return true; }
		virtual bool isRemoteChannel() const { return false; }

		void addAccount(const im::Account& account);
		void removeAccount(const im::Account& account);
		size_t countAccounts() const { return accounts.size(); }

		virtual bool invite(Nick* from, const string& nickname, const string& message);
		virtual bool kick(ChanUser* from, ChanUser* victim, const string& message);
		virtual bool setTopic(Entity* who, const string& message);

		virtual void showBanList(Nick* to);

		virtual void processBan(Nick* from, string pattern, bool add);
	};

}; /* ns irc */

#endif /* IRC_STATUS_CHANNEL_H */
