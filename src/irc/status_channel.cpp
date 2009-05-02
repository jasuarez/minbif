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

#include <algorithm>

#include "irc.h"
#include "status_channel.h"
#include "nick.h"
#include "../util.h"

namespace irc {

StatusChannel::StatusChannel(IRC* irc, string name)
	: Channel(irc, name)
{}

void StatusChannel::addAccount(const im::Account& account)
{
	accounts.push_back(account);
}

void StatusChannel::removeAccount(const im::Account& account)
{
	vector<im::Account>::iterator it = std::find(accounts.begin(), accounts.end(), account);
	if(it != accounts.end())
		accounts.erase(it);
}

void StatusChannel::showBanList(Nick* to)
{
	FOREACH(vector<im::Account>, accounts, account)
	{
		vector<string> deny_list = account->getDenyList();
		FOREACH(vector<string>, deny_list, str)
		{
			to->send(Message(RPL_BANLIST).setSender(irc)
					             .setReceiver(to)
						     .addArg(getName())
						     .addArg(*str + ":" + account->getID())
						     .addArg(to->getLongName())
						     .addArg("0"));
		}
	}
	to->send(Message(RPL_ENDOFBANLIST).setSender(irc)
			                  .setReceiver(to)
					  .addArg(getName())
					  .addArg("End of Channel Ban List"));
}

}; /* ns irc */
