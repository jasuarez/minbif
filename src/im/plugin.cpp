/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon
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

#include "plugin.h"

namespace im
{

Plugin::Plugin()
	: plug(0)
{

}

Plugin::Plugin(PurplePlugin* _plug)
	: plug(_plug)
{

}

string Plugin::getID() const
{
	assert(isValid());
	return plug->info->id;
}

string Plugin::getName() const
{
	assert(isValid());
	return plug->info->name;
}

string Plugin::getSummary() const
{
	assert(isValid());
	return plug->info->summary;
}

string Plugin::getDescription() const
{
	assert(isValid());
	return plug->info->description;
}

bool Plugin::isLoaded() const
{
	assert(isValid());
	return purple_plugin_is_loaded(plug);
}

void Plugin::load()
{
	assert(isValid());
	if (purple_plugin_is_unloadable(plug))
		throw LoadError("Plugin is unloadable");
	purple_plugin_load(plug);
}

void Plugin::unload()
{
	assert(isValid());
	if (purple_plugin_is_unloadable(plug))
		throw LoadError("Plugin is unloadable");
	if (!purple_plugin_unload(plug))
		throw LoadError(string("Unable to unload the plugin: ") + plug->error);
}

}; /* ns im */
