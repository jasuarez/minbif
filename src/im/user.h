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

#ifndef IM_USER_H
#define IM_USER_H

#include <exception>
#include <string>

#include "im.h"

namespace irc
{
	class IRC;
};

/** IM related classes */
namespace im
{
	using std::string;

	class User
	{
	public:
		static User* build(irc::IRC* _irc, string _username);
		User(irc::IRC* _irc, string _username);
		virtual bool exists() = 0;
		virtual bool authenticate(const string password) = 0;
		virtual im::IM* create(const string password);
		virtual im::IM* getIM();
		virtual bool setPassword(const string& password) = 0;
		virtual string getPassword() const = 0;

	protected:
		string username;
		irc::IRC* irc;

		im::IM *im;
	};
};

#endif /* IM_USER_H */
