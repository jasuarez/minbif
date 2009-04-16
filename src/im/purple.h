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

#ifndef IM_PURPLE_H
#define IM_PURPLE_H

#include <exception>
#include <map>
#include <string>

#include "protocol.h"

namespace im
{
	using std::map;
	using std::string;

	class IM;

	class PurpleError : public std::exception {};

	class Purple
	{
		/* Instanciation is forbidden */
		Purple() {}
		~Purple() {}

		static IM* im;

	public:

		static void Init(IM* im);
		static void Uninit();

		/** Get protocols list
		 *
		 * @return  map with first=id, second=Protocol object
		 */
		static map<string, Protocol> getProtocolsList();
	};
};

#endif /* IM_PURPLE_H */
