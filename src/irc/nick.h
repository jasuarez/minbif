/*
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

#ifndef NICK_H
#define NICK_H

#include <string>
using std::string;

class Nick
{
	string nickname, identname, hostname, realname;
	unsigned int flags;

public:

	enum {
		REGISTERED = 1 << 0
	};

	Nick(string nickname, string identname, string hostname, string realname="");
	~Nick();

	string getNickname() const { return nickname; }
	void setNickname(string n) { nickname = n; }

	string getIdentname() const { return identname; }
	void setIdentname(string i) { identname = i; }

	string getHostname() const { return hostname; }
	void setHostname(string h) { hostname = h; }

	string getRealname() const { return realname; }
	void setRealname(string r) { realname = r; }

	void setFlag(unsigned flag) { flags |= flag; }
	void delFlag(unsigned flag) { flags &= ~flag; }
	bool hasFlag(unsigned flag) const { return flags & flag; }
};

#endif /* NICK_H */
