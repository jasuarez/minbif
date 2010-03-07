/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2010 Romain Bignon, Marc Dequènes (Duck)
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

#include <cstring>
#include <cerrno>
#include "auth.h"
#include "core/log.h"
#include "core/util.h"
#include "core/config.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "auth_connection.h"

namespace im
{
AuthConnection::AuthConnection(irc::IRC* _irc, const string& _username)
	: Auth(_irc, _username)
{
}

bool AuthConnection::exists()
{
	return true;
}

bool AuthConnection::authenticate(const string& password)
{
	if (!im::IM::exists(username))
		return false;

	SockWrapper* sockw = irc->getSockWrap();

	b_log[W_DEBUG] << "Authenticating user " << username << " using connection information";
	if (sockw->GetClientUsername() == username)
	{
		im = new im::IM(irc, username);
		return true;
	}

	return false;
}

bool AuthConnection::setPassword(const string& password)
{
	b_log[W_ERR] << "PAM: Password change failed: you are not authenticated using a password ";
	return false;
}

string AuthConnection::getPassword() const
{
	b_log[W_WARNING] << "Cannot fetch current password, you are not authenticated using a password";
	return "";
}

im::IM* AuthConnection::create(const string& password)
{
	if (irc->getSockWrap()->GetClientUsername() != username)
		throw IMError("No connection authentication");

	return Auth::create(password);
}

}; /* namespace im */
