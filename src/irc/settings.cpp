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

#include "settings.h"
#include "im/im.h"
#include "irc.h"
#include "user.h"
#include "../log.h"

namespace irc
{

SettingBase::SettingBase(IRC* _irc, im::IM* _im)
	: irc(_irc),
	  im(_im)
{}

string SettingPassword::getValue() const
{
	return getIM()->getPassword();
}

bool SettingPassword::setValue(string v)
{
	if(v.find(' ') != string::npos || v.size() < 8)
	{
		getIRC()->notice(getIRC()->getUser(), "Password must be at least 8 characters, and does not contain whitespaces.");
		return false;
	}

	getIM()->setPassword(v);
	return true;
}

string SettingTypingNotice::getValue() const
{
	return getIM()->hasTypingNotice() ? "true" : "false";
}

bool SettingTypingNotice::setValue(string v)
{
	if(v == "1" || v == "true")
		getIM()->setTypingNotice(true);
	else if(v == "0" || v == "false")
		getIM()->setTypingNotice(false);
	else
	{
		getIRC()->notice(getIRC()->getUser(), "Value must be 'true' or 'false'");
		return false;
	}
	return true;
}

string SettingAwayIdle::getValue() const
{
	return getIM()->hasAwayIdle() ? "true" : "false";
}

bool SettingAwayIdle::setValue(string v)
{
	if(v == "1" || v == "true")
		getIM()->setAwayIdle(true);
	else if(v == "0" || v == "false")
		getIM()->setAwayIdle(false);
	else
	{
		getIRC()->notice(getIRC()->getUser(), "Value must be 'true' or 'false'");
		return false;
	}
	return true;
}

string SettingMinbif::getValue() const
{
	return "Minbif Is Not Bitlbee Fool";
}

bool SettingMinbif::setValue(string)
{
	getIRC()->notice(getIRC()->getUser(), "Wilmer, I see you!");
	return false;
}

string SettingLogLevel::getValue() const
{
	return b_log.formatLoggedFlags();
}

bool SettingLogLevel::setValue(string s)
{
	b_log.setLoggedFlags(s, b_log.toSyslog());
	return true;
}

}; /* ns irc */
