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

#ifndef IRC_SETTINGS_H
#define IRC_SETTINGS_H

#include <string>

namespace im
{
	class IM;
};

namespace irc
{
	using std::string;
	class IRC;

	class SettingBase
	{
		IRC* irc;
		im::IM* im;

	protected:
		im::IM* getIM() const { return im; }
		IRC* getIRC() const { return irc; }

	public:

		SettingBase(IRC* irc, im::IM* im);
		virtual ~SettingBase() {}

		virtual string getValue() const = 0;
		virtual bool setValue(string v) = 0;
	};

	class SettingPassword : public SettingBase
	{
	public:
		SettingPassword(IRC* irc, im::IM* im) : SettingBase(irc, im) {}

		virtual string getValue() const;
		virtual bool setValue(string v);
	};

	class SettingTypingNotice : public SettingBase
	{
	public:
		SettingTypingNotice(IRC* irc, im::IM* im) : SettingBase(irc, im) {}

		virtual string getValue() const;
		virtual bool setValue(string v);
	};

	class SettingAwayIdle : public SettingBase
	{
	public:
		SettingAwayIdle(IRC* irc, im::IM* im) : SettingBase(irc, im) {}

		virtual string getValue() const;
		virtual bool setValue(string v);
	};

	class SettingMinbif : public SettingBase
	{
	public:
		SettingMinbif(IRC* irc, im::IM* im) : SettingBase(irc, im) {}
		virtual string getValue() const;
		virtual bool setValue(string v);
	};

	class SettingLogLevel : public SettingBase
	{
	public:
		SettingLogLevel(IRC* irc, im::IM* im) : SettingBase(irc, im) {}
		virtual string getValue() const;
		virtual bool setValue(string v);
	};

}; /* ns irc */

#endif /* IRC_SETTINGS_H */
