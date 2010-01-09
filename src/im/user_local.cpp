/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2010 Marc Dequènes (Duck)
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
#include "user.h"
#include "core/log.h"
#include "core/util.h"
#include "core/config.h"
#include "irc/irc.h"
#include "user_local.h"

namespace im
{
UserLocal::UserLocal(irc::IRC* _irc, string _username)
	: User(_irc, _username)
{
}

bool UserLocal::exists()
{
	return im::IM::exists(username);
}

bool UserLocal::authenticate(const string password)
{
	if (!im::IM::exists(username))
		return false;

	im = new im::IM(irc, username);

	b_log[W_INFO] << "Authenticating user " << im->getUsername() << " using local database";
	return im->getPassword() == password;
}
}; /* namespace im */
