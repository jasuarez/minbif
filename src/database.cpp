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

#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>

#include "database.h"
#include "log.h"

/* STATIC */
string Database::path;

void Database::setPath(const string& _path)
{
	DIR* d;

	path = _path;
	if(!(d = opendir(path.c_str())))
	{
		if(mkdir(path.c_str(), 0700) < 0)
		{
			b_log[W_ERR] << "Unable to create directory '" << path << "': " << strerror(errno);
			throw DatabaseError();
		}
	}
	else
		closedir(d);
}

bool Database::exists(const string& username)
{
	DIR* d;
	if(!(d = opendir((path + "/" + username).c_str())))
		return false;

	closedir(d);
	return true;
}

/* DATABASE OBJECT */

Database::Database(string _username)
	: username(_username)
{}
