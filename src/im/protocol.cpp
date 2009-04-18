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

#include <cassert>
#include <cstring>
#include "protocol.h"

namespace im {

Protocol::Protocol()
	: plugin(NULL)
{}

Protocol::Protocol(PurplePlugin* _plugin)
	: plugin(_plugin)
{}

bool Protocol::operator==(const Protocol& proto) const
{
	return proto.getPurpleID() == getPurpleID();
}

bool Protocol::operator!=(const Protocol& proto) const
{
	return proto.getPurpleID() != getPurpleID();
}

string Protocol::getName() const
{
	assert(plugin);
	return plugin->info->name;
}

string Protocol::getID() const
{
	assert(plugin);
	if(!strncmp(plugin->info->id, "prpl-", 5))
		return plugin->info->id + 5;
	else
		return plugin->info->id;
}

string Protocol::getPurpleID() const
{
	assert(plugin);
	return plugin->info->id;
}

}; /* namespace im */
