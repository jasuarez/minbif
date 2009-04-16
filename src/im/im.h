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

#ifndef IM_IM_H
#define IM_IM_H

#include <exception>
#include <string>
#include <map>

using std::string;
using std::map;

class IMError : public std::exception {};

class IM
{
	static string path;

public:

	static void setPath(const string& path);
	static bool exists(const string& username);

private:

	string username;
	string user_path;
public:

	IM(string username);
	~IM();

	string getUserPath() const { return user_path; }

	void setPassword(const string& password);
	string getPassword() const;

	map<string, string> getProtocolsList() const;
};

#endif /* IM_IM_H */
