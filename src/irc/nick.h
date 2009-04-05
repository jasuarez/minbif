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
	string nickname, identname, hostname;

public:

	Nick(string nickname, string identname, string hostname);
	~Nick();

	string getNickname() const { return nickname; }
	string getIdentname() const { return identname; }
	string getHostname() const { return hostname; }
};

#endif /* NICK_H */
