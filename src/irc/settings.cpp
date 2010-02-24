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
#include "core/util.h"
#include "core/log.h"

namespace irc
{

SettingBase::SettingBase(IRC* _irc, im::IM* _im)
	: irc(_irc),
	  im(_im)
{}

string SettingPassword::getValue() const
{
	return getIRC()->getIMAuth()->getPassword();
}

bool SettingPassword::setValue(string v)
{
	return getIRC()->getIMAuth()->setPassword(v);
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

string SettingAcceptNoBuddiesMessages::getValue() const
{
	return getIM()->hasAcceptNoBuddiesMessages() ? "true" : "false";
}

bool SettingAcceptNoBuddiesMessages::setValue(string v)
{
	if(v == "1" || v == "true")
		getIM()->setAcceptNoBuddiesMessages(true);
	else if(v == "0" || v == "false")
		getIM()->setAcceptNoBuddiesMessages(false);
	else
	{
		getIRC()->notice(getIRC()->getUser(), "Value must be 'true' or 'false'");
		return false;
	}
	return true;
}

string SettingVoicedBuddies::getValue() const
{
	return getIM()->hasVoicedBuddies() ? "true" : "false";
}

bool SettingVoicedBuddies::setValue(string v)
{
	if(v == "1" || v == "true")
		getIM()->setVoicedBuddies(true);
	else if(v == "0" || v == "false")
		getIM()->setVoicedBuddies(false);
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

string SettingProxy::getValue() const
{
	return purple_prefs_get_string("/purple/proxy/type");
}

bool SettingProxy::setValue(string s)
{
	static const char* l[] = {"none", "sock4", "sock5", "http", "envvar"};

	for(unsigned i = 0; i < sizeof(l) / sizeof(*l); ++i)
		if(s == l[i])
		{
			purple_prefs_set_string("/purple/proxy/type", s.c_str());
			return true;
		}

	getIRC()->notice(getIRC()->getUser(), "Available values are: none, sock4, sock5, http, envvar");
	return false;
}

string SettingProxyHost::getValue() const
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	const char* s = proxy ? purple_proxy_info_get_host(proxy) : "";
	return s ? s : "";
}

bool SettingProxyHost::setValue(string s)
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	if(proxy)
	{
		purple_proxy_info_set_host(proxy, s.c_str());
		return true;
	}
	else
		return false;
}

string SettingProxyPort::getValue() const
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	return proxy ? t2s(purple_proxy_info_get_port(proxy)) : "";
}

bool SettingProxyPort::setValue(string s)
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	if(proxy)
	{
		purple_proxy_info_set_port(proxy, s2t<int>(s));
		return true;
	}
	else
		return false;
}

string SettingProxyUsername::getValue() const
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	const char* s = proxy ? purple_proxy_info_get_username(proxy) : "";
	return s ? s : "";
}

bool SettingProxyUsername::setValue(string s)
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	if(proxy)
	{
		purple_proxy_info_set_username(proxy, s.c_str());
		return true;
	}
	else
		return false;
}

string SettingProxyPassword::getValue() const
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	const char* s = proxy ? purple_proxy_info_get_password(proxy) : "";
	return s ? s : "";
}

bool SettingProxyPassword::setValue(string s)
{
	PurpleProxyInfo* proxy = purple_global_proxy_get_info();
	if(proxy)
	{
		purple_proxy_info_set_password(proxy, s.c_str());
		return true;
	}
	else
		return false;
}


}; /* ns irc */
