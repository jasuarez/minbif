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

#ifndef IM_PLUGIN_H
#define IM_PLUGIN_H

#include <purple.h>
#include <string>

#include "core/exception.h"

namespace im {

	using std::string;

	class Plugin
	{
		PurplePlugin* plug;

	public:

		STREXCEPTION(LoadError);

		Plugin();
		Plugin(PurplePlugin* plug);

		bool isValid() const { return plug != NULL; }

		string getID() const;
		string getName() const;
		string getSummary() const;
		string getDescription() const;

		bool isLoaded() const;

		void load();
		void unload();
	};

}; /* ns im */

#endif /* IM_PLUGIN_H */
