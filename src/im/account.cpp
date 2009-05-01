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

#include <cassert>

#include "account.h"
#include "buddy.h"
#include "purple.h"
#include "../log.h"
#include "../version.h"
#include "im.h"
#include "irc/irc.h"
#include "irc/status_channel.h"

namespace im {

Account::Account()
	: account(NULL)
{}

Account::Account(PurpleAccount* _account, Protocol _proto)
	: account(_account),
	  proto(_proto)
{
	if(!proto.isValid())
		proto = Purple::getProtocolByPurpleID(account->protocol_id);
}

bool Account::operator==(const Account& account) const
{
	return this->isValid() && account.isValid() && account.account == this->account;
}

bool Account::operator!=(const Account& account) const
{
	return !isValid() || !account.isValid() || account.account != this->account;
}


string Account::getUsername() const
{
	assert(isValid());
	return account->username;
}

string Account::getID() const
{
	assert(isValid());
	string n = purple_account_get_ui_string(account, BITLBEE_VERSION_NAME, "id", "");
	if(n.empty())
		n = Purple::getNewAccountName(proto);
	return n;
}

string Account::getStatusChannel() const
{
	assert(isValid());
	string n = purple_account_get_ui_string(account, BITLBEE_VERSION_NAME, "channel", "");
	return n;
}

void Account::setStatusChannel(string c)
{
	assert(isValid());
	purple_account_set_ui_string(account, BITLBEE_VERSION_NAME, "channel", c.c_str());
}

string Account::getServername() const
{
	assert(isValid());
	return getUsername() + ":" + getID();
}

bool Account::isConnected() const
{
	assert(isValid());
	return purple_account_is_connected(account);
}

bool Account::isConnecting() const
{
	assert(isValid());
	return purple_account_is_connecting(account);
}

vector<Buddy> Account::getBuddies() const
{
	assert(isValid());
	vector<Buddy> buddies;
	return buddies;
}

void Account::connect() const
{
	assert(isValid());
	purple_account_set_enabled(account, BITLBEE_VERSION_NAME, true);
}

void Account::disconnect() const
{
	assert(isValid());
	purple_account_set_enabled(account, BITLBEE_VERSION_NAME, false);
}

void Account::createStatusChannel() const
{
	assert(isValid());

	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::Channel* chan;
	string channame = getStatusChannel();

	if(!irc::Channel::isStatusChannel(channame))
		return;

	chan = irc->getChannel(channame);
	if(!chan)
	{
		chan = new irc::StatusChannel(irc, channame);
		irc->addChannel(chan);
	}
}

void Account::addBuddy(string username, string group) const
{
	assert(isValid());
	assert(username.empty() == false);
	assert(group.empty() == false);

	PurpleGroup* grp = purple_find_group(group.c_str());
	if (!grp)
	{
		grp = purple_group_new(group.c_str());
		purple_blist_add_group(grp, NULL);
	}

	PurpleBuddy* buddy = purple_buddy_new(account, username.c_str(), username.c_str());
	purple_blist_add_buddy(buddy, NULL, grp, NULL);
	purple_account_add_buddy(account, buddy);
}

void Account::removeBuddy(Buddy buddy) const
{
	purple_account_remove_buddy(account, buddy.getPurpleBuddy(), buddy.getPurpleGroup());
	purple_blist_remove_buddy(buddy.getPurpleBuddy());
}

/* STATIC */

PurpleConnectionUiOps Account::conn_ops =
{
	Account::connecting,
        Account::connected,
        Account::disconnected,
        NULL, /* notice */
        NULL,
        NULL, /* network_connected */
        NULL, /* network_disconnected */
        Account::disconnect_reason,
        NULL,
        NULL,
        NULL
};

PurpleAccountUiOps Account::acc_ops =
{
        NULL,//notify_added,
        NULL,
        NULL,//request_add,
        NULL,//finch_request_authorize,
        NULL,//finch_request_close,
        NULL,
        NULL,
        NULL,
        NULL
};


void* Account::getHandler()
{
	static int handler;

	return &handler;
}

void Account::init()
{
	purple_connections_set_ui_ops(&conn_ops);
	purple_accounts_set_ui_ops(&acc_ops);
	purple_signal_connect(purple_accounts_get_handle(), "account-added",
				getHandler(), PURPLE_CALLBACK(account_added),
				NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-removed",
				getHandler(), PURPLE_CALLBACK(account_removed),
				NULL);

	map<string, Account> accounts = Purple::getAccountsList();
	for(map<string, Account>::iterator it = accounts.begin(); it != accounts.end(); ++it)
		it->second.createStatusChannel();
}

void Account::account_added(PurpleAccount* account)
{
}

void Account::account_removed(PurpleAccount* account)
{
}

void Account::connecting(PurpleConnection *gc,
		const char *text,
		size_t step,
		size_t step_count)
{
	Account account = Account(gc->account);

	if(!step)
		b_log[W_INFO|W_SNO] << "Connection to " << account.getServername() << " in progress...";
	else
		b_log[W_INFO|W_SNO] << "" << account.getID() << "(" << step << "/" << step_count-1 << "): " << text;
}

void Account::connected(PurpleConnection* gc)
{
	Account account = Account(gc->account);
	irc::IRC* irc = Purple::getIM()->getIRC();

	b_log[W_INFO|W_SNO] << "Connection to " << account.getServername() << " established!";
	irc->addServer(new irc::RemoteServer(irc, account));
}

void Account::disconnected(PurpleConnection* gc)
{
	Account account = Account(gc->account);

	b_log[W_INFO|W_SNO] << "Closing link with " << account.getServername();
	Purple::getIM()->getIRC()->removeServer(account.getServername());
}

void Account::disconnect_reason(PurpleConnection *gc,
				PurpleConnectionError reason,
				const char *text)
{
	b_log[W_ERR|W_SNO] << "Error(" << Account(gc->account).getServername() << "): " << text;
}

}; /* namespace im */
