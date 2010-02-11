/*
 * Minbif - IRC instant messaging gateway
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

#ifndef IM_ROOMLIST_H
#define IM_ROOMLIST_H

#include <purple.h>
#include <string>

namespace im {

	using std::string;
	class RoomList
	{
		PurpleRoomlist* rlist;

		static PurpleRoomlistUiOps ui_ops;
		static void create(PurpleRoomlist* list);
		static void set_fields(PurpleRoomlist* list, GList* fields);
		static void add_room(PurpleRoomlist*, PurpleRoomlistRoom* room);
		static void in_progress(PurpleRoomlist* list, gboolean flag);
		static void destroy(PurpleRoomlist* list);

	public:

		static void init();
		static void uninit();

		RoomList(PurpleRoomlist* rl);
		~RoomList();
	};

} /* ns im */

#endif /* IM_ROOMLIST_H */
