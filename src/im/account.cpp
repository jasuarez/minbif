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

#include <cassert>

#include "account.h"
#include "purple.h"
#include "../log.h"
#include "../version.h"
#include "im.h"
#include "irc/irc.h"

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

string Account::getUsername() const
{
	assert(account);
	return account->username;
}

string Account::getID() const
{
	string n = purple_account_get_ui_string(account, BITLBEE_VERSION_NAME, "id", "");
	if(n.empty())
		n = Purple::getNewAccountName(proto);
	return n;
}


/* STATIC */

void* Account::getHandler()
{
	static int handler;

	return &handler;
}

void Account::init()
{
	purple_signal_connect(purple_accounts_get_handle(), "account-added",
				getHandler(), PURPLE_CALLBACK(account_added),
				NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-removed",
				getHandler(), PURPLE_CALLBACK(account_removed),
				NULL);
}

void Account::account_added(PurpleAccount* account)
{
	Purple::getIM()->getIRC()->addServer(new irc::RemoteServer(Account(account)));
}

void Account::account_removed(PurpleAccount* account)
{
	Account a = Account(account);
	Purple::getIM()->getIRC()->removeServer(a.getUsername() + ":" + a.getID());
}

}; /* namespace im */
