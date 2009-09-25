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

#ifndef UTIL_H
#define UTIL_H

#include <libpurple/purple.h>
#include <string>
#include <sstream>
using std::string;

string stringtok(string &in, const char * const delimiters);

template<typename T>
T s2t(const std::string & Str)
{
	T Dest;
	std::istringstream iss( Str );
	iss >> Dest;
	return Dest;
}

template<typename T>
std::string t2s( const T & Value )
{
	std::ostringstream oss;
	oss << Value;
	return oss.str();
}


guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function,
                                                           gpointer data);

string strupper(string s);
string strlower(string s);

bool is_ip(const char *ip);

#define FOREACH(t, v, it) \
	for(t::iterator it = v.begin(); it != v.end(); ++it)

#endif /* UTIL_H */
