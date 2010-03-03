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
#include "auth.h"
#include "core/log.h"
#include "core/util.h"
#include "core/config.h"
#include "irc/irc.h"
#include "auth_local.h"
#include "auth_connection.h"
#ifdef HAVE_PAM
#  include "auth_pam.h"
#endif

namespace im
{
Auth* Auth::build(irc::IRC* _irc, string _username)
{
	if (conf.GetSection("aaa")->GetItem("use_connection")->Boolean())
		return new AuthConnection(_irc, _username);
#ifdef HAVE_PAM
	else if (conf.GetSection("aaa")->GetItem("use_pam")->Boolean())
		return new AuthPAM(_irc, _username);
#endif
	return new AuthLocal(_irc, _username);
}

Auth::Auth(irc::IRC* _irc, string _username)
	: username(_username),
	  irc(_irc)
{
	im = NULL;
}

im::IM* Auth::create(const string password)
{
	im = new im::IM(irc, username);
	im->setPassword(password);

	return im;
}
}; /* namespace im */
