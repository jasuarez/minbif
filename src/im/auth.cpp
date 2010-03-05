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
#include <vector>
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

template<typename T>
static Auth* authFactory(irc::IRC* irc, const string& username)
{
	return new T(irc, username);
}

static struct
{
	const char* config_param;
	Auth* (*factory) (irc::IRC* irc, const string& username);
} auth_mechanisms[] = {
	{ "use_connection", authFactory<AuthConnection> },
#ifdef HAVE_PAM
	{ "use_pam",        authFactory<AuthPAM> },
#endif
	{ "use_local",      authFactory<AuthLocal> }
};

vector<Auth*> Auth::getMechanisms(irc::IRC* irc, const string& username)
{
	vector<Auth*> mechanisms;
	for(size_t i = 0; i < (sizeof auth_mechanisms / sizeof *auth_mechanisms); ++i)
		if (conf.GetSection("aaa")->GetItem(auth_mechanisms[i].config_param)->Boolean())
			mechanisms.push_back(auth_mechanisms[i].factory(irc, username));
	return mechanisms;
}

Auth* Auth::validate(irc::IRC* irc, const string& username, const string& password)
{
	vector<Auth*> mechanisms = getMechanisms(irc, username);
	Auth* mech_ok = NULL;

	if (mechanisms.empty())
	{
		b_log[W_ERR] << "Login disabled (please consult your administrator)";
		throw IMError();
	}

	for (vector<Auth*>::iterator m = mechanisms.begin(); m != mechanisms.end(); ++m)
	{
		if ((mech_ok == NULL) && (*m)->exists() && (*m)->authenticate(password))
			mech_ok = *m;
		else
			delete *m;
	}

	return mech_ok;
}

Auth* Auth::generate(irc::IRC* irc, const string& username, const string& password)
{
	vector<Auth*> mechanisms = getMechanisms(irc, username);
	Auth* mech_ok = NULL;

	if (mechanisms.empty())
	{
		b_log[W_ERR] << "Private server: account creation disabled";
		throw IMError();
	}

	for (vector<Auth*>::iterator m = mechanisms.begin(); m != mechanisms.end(); ++m)
	{
		if ((mech_ok == NULL) && (*m)->create(password))
			mech_ok = *m;
		else
			delete *m;
	}

	return mech_ok;
}

Auth::Auth(irc::IRC* _irc, const string& _username)
	: username(_username),
	  irc(_irc)
{
	im = NULL;
}

im::IM* Auth::create(const string& password)
{
	if (exists())
		return NULL;

	b_log[W_DEBUG] << "Creating user " << username;

	im = new im::IM(irc, username);
	setPassword(password);

	return im;
}
}; /* namespace im */
