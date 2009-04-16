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

#include "account.h"
#include "../log.h"

namespace im {

Account::Account()
	: account(NULL)
{}

Account::Account(PurpleAccount* _account)
	: account(_account)
{}

string Account::getUsername() const
{
	return account->username;
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
}

void Account::account_added(PurpleAccount* account)
{
	b_log[W_INFO] << "Account added " << account;
}

}; /* namespace im */
