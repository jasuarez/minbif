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
#include "im/im.h"
#include "im/account.h"
#include "core/util.h"
#include "core/log.h"

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

string StatusChannel::getMaskFromName(const string& name, const im::Account& acc) const
{
	string ban = "*!" + name;
	if(ban.find('@') == string::npos)
		ban += "@" + acc.getID();
	else
		ban += ":" + acc.getID();
	return ban;
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
						     .addArg(getMaskFromName(*str, *account))
						     .addArg(account->getServername())
						     .addArg("0"));
		}
	}
	to->send(Message(RPL_ENDOFBANLIST).setSender(irc)
			                  .setReceiver(to)
					  .addArg(getName())
					  .addArg("End of Channel Ban List"));
}

void StatusChannel::processBan(Nick* from, string pattern, bool add)
{
	bool (im::Account::*func) (const string&) const;
	string mode;

	if (add)
	{
		mode = "+b";
		func = &im::Account::deny;
	}
	else
	{
		mode = "-b";
		func = &im::Account::allow;
	}

	vector<Nick*> results = irc->matchNick(pattern);
	if(!results.empty())
	{
		for(vector<Nick*>::iterator n = results.begin(); n != results.end(); ++n)
		{
			Buddy* nick = dynamic_cast<Buddy*>(*n);
			if (!nick)
				continue;

			im::Buddy buddy = nick->getBuddy();
			if ((buddy.getAccount().*func)(buddy.getName()))
				broadcast(Message(MSG_MODE).setSender(from)
							   .setReceiver(this)
							   .addArg(mode)
							   .addArg(getMaskFromName(nick->getBuddy().getName(), nick->getBuddy().getAccount())));
		}
	}
	else
	{
		im::Account acc;
		string accid = pattern;
		pattern = stringtok(accid, ":");
		if (accid.empty())
		{
			accid = pattern;
			pattern = stringtok(accid, "@");
		}

		vector<im::Account>::iterator it;
		for(it = accounts.begin(); it != accounts.end() && it->getID() != accid; ++it)
			;

		if (it != accounts.end())
			acc = *it;
		else if(accounts.size() == 1)
			acc = accounts.front();
		else
		{
			from->send(Message(ERR_NOSUCHNICK).setSender(irc)
							  .setReceiver(from)
							  .addArg(accid.empty() ? pattern : accid)
							  .addArg(accid.empty() ? "Please sufix mask with ':accountID'" : "No such account"));
			return;
		}

		if (pattern.find('!') != string::npos)
			stringtok(pattern, "!");

		if ((acc.*func)(pattern))
			broadcast(Message(MSG_MODE).setSender(from)
						   .setReceiver(this)
						   .addArg(mode)
						   .addArg(getMaskFromName(pattern, acc)));

	}
}

bool StatusChannel::invite(Nick* from, const string& nickname, const string& message)
{
	string acc = nickname;
	string username = stringtok(acc, ":");
	im::Account account;
	if(acc.empty())
		account = irc->getIM()->getAccountFromChannel(getName());
	else
		account = irc->getIM()->getAccount(acc);

	if(!account.isValid())
	{
		from->send(Message(ERR_NOSUCHCHANNEL).setSender(irc)
						     .setReceiver(from)
						     .addArg(nickname)
						     .addArg("No such channel"));
		return false;
	}
	account.addBuddy(username, "minbif");
	return true;
}

bool StatusChannel::kick(ChanUser* from, ChanUser* victim, const string& message)
{
	Buddy* buddy = dynamic_cast<Buddy*>(victim->getNick());
	if(!buddy)
	{
		from->getNick()->send(Message(ERR_CHANOPRIVSNEEDED).setSender(irc)
						               .setReceiver(from)
							       .addArg(getName())
						               .addArg("Permission denied: you can only kick a buddy"));
		return false;
	}

	RemoteServer* rt = dynamic_cast<RemoteServer*>(buddy->getServer());
	if(!rt)
	{
		irc->notice(from->getNick(), victim->getName() + " is not on a remote server");
		return false;
	}
	string reason = "Removed from buddy list";
	if(message.empty() == false)
		reason += ": " + message;

	buddy->kicked(this, from, reason);
	rt->getAccount().removeBuddy(buddy->getBuddy());
	return true;
}

bool StatusChannel::setTopic(Entity* from, const string& message)
{
	for(vector<im::Account>::iterator acc = accounts.begin(); acc != accounts.end(); ++acc)
		acc->setStatus(PURPLE_STATUS_AVAILABLE, message);
	return Channel::setTopic(from, message);
}

}; /* ns irc */
