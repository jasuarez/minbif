/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon, Marc Dequènes (Duck)
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
#include <dirent.h>
#include <sys/stat.h>

#include "im.h"
#include "purple.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "core/log.h"
#include "core/util.h"

namespace im
{

/* STATIC */
string IM::path;

void IM::setPath(const string& _path)
{
	DIR* d;

	path = _path;
	if(!(d = opendir(path.c_str())))
	{
		if(mkdir(path.c_str(), 0700) < 0)
			throw IMError("Unable to create directory '" + path + "': " + strerror(errno));
	}
	else
		closedir(d);
}

bool IM::exists(const string& username)
{
	DIR* d;
	if(!(d = opendir((path + "/" + username).c_str())))
		return false;

	closedir(d);
	return true;
}

/* METHODS */

IM::IM(irc::IRC* _irc, string _username)
	: username(_username),
	  user_path(path + "/" + username),
	  irc(_irc)
{
	DIR* d;

	if(!(d = opendir(user_path.c_str())))
	{
		if(mkdir(user_path.c_str(), 0700) < 0)
			throw IMError("Unable to create user directory '" + user_path + "': " + strerror(errno));
	}
	else
		closedir(d);

	try
	{
		Purple::init(this);
	}
	catch(PurpleError &e)
	{
		throw IMError(e.Reason());
	}
}

IM::~IM()
{
	purple_core_quit();
}

void IM::restore()
{
	if (!purple_prefs_get_bool("/purple/savedstatus/startup_current_status"))
		purple_savedstatus_activate(purple_savedstatus_get_startup());
	purple_accounts_restore_current_statuses();
}

void IM::setPassword(const string& password)
{
	purple_prefs_set_string("/minbif/password", password.c_str());
}

string IM::getPassword() const
{
	return purple_prefs_get_string("/minbif/password");
}

void IM::setTypingNotice(bool enabled)
{
	purple_prefs_set_int("/minbif/typing_notice", enabled ? 1 : 0);
}

bool IM::hasTypingNotice() const
{
	return purple_prefs_get_int("/minbif/typing_notice");
}

void IM::setAcceptNoBuddiesMessages(bool enabled)
{
	purple_prefs_set_bool("/minbif/accept_nobuddies_messages", enabled);
}

bool IM::hasAcceptNoBuddiesMessages() const
{
	return purple_prefs_get_bool("/minbif/accept_nobuddies_messages");
}

void IM::setVoicedBuddies(bool enabled)
{
	if(hasVoicedBuddies() != enabled)
	{
		map<string, Account> accs = getAccountsList();

		purple_prefs_set_bool("/minbif/voiced_buddies", enabled);
		for(map<string, Account>::iterator it = accs.begin(); it != accs.end(); ++it)
			it->second.updatedAllBuddies();

	}
}

bool IM::hasVoicedBuddies() const
{
	return purple_prefs_get_bool("/minbif/voiced_buddies");
}

void IM::setServerAliases(bool enabled)
{
	purple_prefs_set_bool("/minbif/server_aliases", enabled);
}

bool IM::hasServerAliases() const
{
	return purple_prefs_get_bool("/minbif/server_aliases");
}

void IM::setAwayIdle(bool enabled)
{
	purple_prefs_set_bool("/purple/away/away_when_idle", enabled);
}

bool IM::hasAwayIdle() const
{
	return purple_prefs_get_bool("/purple/away/away_when_idle");
}

map<string, Protocol> IM::getProtocolsList() const
{
	return Purple::getProtocolsList();
}

Protocol IM::getProtocol(string id) const
{
	map<string, Protocol> plist = getProtocolsList();
	map<string, Protocol>::iterator it = plist.find(id);
	if(it == plist.end())
		throw ProtocolUnknown();
	else
		return it->second;
}

map<string, Account> IM::getAccountsList() const
{
	return Purple::getAccountsList();
}

Account IM::getAccount(string name) const
{
	map<string, Account> alist = getAccountsList();
	for(map<string, Account>::iterator it = alist.begin(); it != alist.end(); ++it)
		if(it->second.getServername() == name || it->second.getID() == name)
			return it->second;
	return Account();
}

Account IM::getAccountFromChannel(string name) const
{
	map<string, Account> alist = getAccountsList();
	for(map<string, Account>::iterator it = alist.begin(); it != alist.end(); ++it)
		if(it->second.getStatusChannelName() == name)
			return it->second;
	return Account();
}


Account IM::addAccount(const Protocol& proto, const string& username, const Protocol::Options& options, bool register_account)
{
	return Purple::addAccount(proto, username, options, register_account);

}

void IM::delAccount(Account user)
{
	Purple::delAccount(user.getPurpleAccount());
}

void IM::setBuddyIcon(const string& path)
{
	map<string, Account> alist = getAccountsList();
	for(map<string, Account>::iterator it = alist.begin(); it != alist.end(); ++it)
		it->second.setBuddyIcon(path);
}

string IM::getBuddyIconPath() const
{
	return string(purple_user_dir()) + "/buddy_icon/";
}

bool IM::setStatus(string away)
{
	PurpleStatusPrimitive prim = PURPLE_STATUS_AVAILABLE;
	if(away.empty() == false)
	{
		string s = strlower(away);
		int i;
		/* Looking for an existant status equivalent to the away message. */
		for(i = 0;
		    (
		      i < PURPLE_STATUS_NUM_PRIMITIVES &&
		      strlower(purple_primitive_get_name_from_type((PurpleStatusPrimitive)i)) != s &&
		      purple_primitive_get_id_from_type((PurpleStatusPrimitive)i) != s
		    );
		    ++i)
			;

		if(i >= PURPLE_STATUS_NUM_PRIMITIVES)
		{
			/* If the status does not exist, set the AWAY status and set the message
			 * given in the \b away string as the status message. */
			prim = PURPLE_STATUS_AWAY;
		}
		else
		{
			prim = (PurpleStatusPrimitive)i;
			away.clear(); /* do not change the saved status message. */
		}
	}

	map<string, Account> alist = getAccountsList();
	for(map<string, Account>::iterator it = alist.begin(); it != alist.end(); ++it)
		it->second.setStatus(prim, away);

	return true;
}

}; /* namespace im */
