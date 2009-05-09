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

#include <algorithm>

#include "irc.h"
#include "status_channel.h"
#include "nick.h"
#include "buddy.h"
#include "im/account.h"
#include "../util.h"
#include "../log.h"

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
			string ban = "*!" + *str;
			if(ban.find('@') == string::npos)
				ban += "@" + account->getID();
			else
				ban += ":" + account->getID();

			to->send(Message(RPL_BANLIST).setSender(irc)
					             .setReceiver(to)
						     .addArg(getName())
						     .addArg(ban)
						     .addArg(account->getServername())
						     .addArg("0"));
		}
	}
	to->send(Message(RPL_ENDOFBANLIST).setSender(irc)
			                  .setReceiver(to)
					  .addArg(getName())
					  .addArg("End of Channel Ban List"));
}

void StatusChannel::processAddBan(Nick* from, string nick, string ident, string host, string accid)
{
	string deny;
	im::Account account;

	if(nick.empty() == false && nick != "*")
	{
		/* User gives a nick, perhaps just 'nick', perhaps 'nick!user@host:acc', but in
		 * this case we only consider this is a connected nick, and find the banned mask
		 * and account from his buddy.
		 */
		Buddy* banned = dynamic_cast<Buddy*>(irc->getNick(nick));
		if(!banned || banned->getBuddy().getAccount().getStatusChannel() != getName())
		{
			from->send(Message(ERR_NOSUCHNICK).setSender(irc)
					                  .setReceiver(from)
							  .addArg(nick)
							  .addArg("No such nick."));
			return;
		}
		deny = banned->getBuddy().getName();
		account = banned->getBuddy().getAccount();
	}
	else
	{
		/* No nick given, trying to get account and banned mask. */
		if(!ident.empty() && ident[0] == '*')
			ident = ident.substr(1);

		vector<im::Account>::iterator it;
		for(it = accounts.begin(); it != accounts.end() && it->getID() != accid; ++it)
			;

		if(it == accounts.end())
		{
			/* So bad, accid is not a valid account ID. If there is only
			 * one account, we can consider this is it.
			 * In othes case we warn user.
			 */
			if(accounts.size() == 1)
			{
				account = accounts.front();
				if(host.empty())
					host = accid;
				accid = account.getID();
			}
			else
			{
				from->send(Message(ERR_NOSUCHNICK).setSender(irc)
								  .setReceiver(from)
								  .addArg(host)
								  .addArg("Please prefix mask with ':accountID'"));
				return;
			}
		}
		else
			account = *it;

		deny = ident;
		if(host.empty() == false)
			deny += "@" + host;
	}

	/* Hm, there is so nothing to ban?? */
	if(deny.empty())
		return;

	account.deny(deny);

	string ban = "*!" + deny;
	if(ban.find('@') == string::npos)
		ban += "@" + account.getID();
	else
		ban += ":" + account.getID();

	broadcast(Message(MSG_MODE).setSender(from)
			           .setReceiver(this)
				   .addArg("+b")
				   .addArg(ban));
}

void StatusChannel::processRemoveBan(Nick* from, string nick, string ident, string host, string accid)
{
	string deny;
	im::Account account;

	if(nick.empty() == false && nick != "*")
	{
		Buddy* banned = dynamic_cast<Buddy*>(irc->getNick(nick));
		if(!banned || banned->getBuddy().getAccount().getStatusChannel() != getName())
		{
			from->send(Message(ERR_NOSUCHNICK).setSender(irc)
					                  .setReceiver(from)
							  .addArg(nick)
							  .addArg("No such nick."));
			return;
		}
		deny = banned->getBuddy().getName();
		account = banned->getBuddy().getAccount();

	}
	else
	{
		vector<im::Account>::iterator it;
		for(it = accounts.begin(); it != accounts.end() && it->getID() != accid; ++it)
			;

		if(it == accounts.end())
			return;

		account = *it;
		deny = ident;
		if(!host.empty())
			deny += "@" + host;
	}

	account.allow(deny);

	string ban = "*!" + deny;
	if(ban.find('@') == string::npos)
		ban += "@" + account.getID();
	else
		ban += ":" + account.getID();

	broadcast(Message(MSG_MODE).setSender(from)
			           .setReceiver(this)
				   .addArg("-b")
				   .addArg(ban));

}

}; /* ns irc */
