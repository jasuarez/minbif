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

#ifndef IM_AUTH_CONNECTION_H
#define IM_AUTH_CONNECTION_H

#include "auth.h"

/** IM related classes */
namespace im
{
	using std::string;

	class AuthConnection : public Auth
	{
	public:
		AuthConnection(irc::IRC* _irc, const string& _username);
		bool exists();
		bool authenticate(const string& password);
		bool setPassword(const string& password);
		string getPassword() const;
	};
};

#endif /* IM_AUTH_CONNECTION_H */
