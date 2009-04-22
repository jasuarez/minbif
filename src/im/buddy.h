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

#ifndef IM_BUDDY_H
#define IM_BUDDY_H

#include <libpurple/purple.h>
#include <string>

#include "caca_image.h"

namespace im
{
	using std::string;

	class Account;
	class Buddy
	{
		PurpleBuddy* buddy;

		static void* getHandler();
		static PurpleBlistUiOps blist_ui_ops;
		static void update_node(PurpleBuddyList *list, PurpleBlistNode *node);

	public:

		static void init();

		Buddy();
		Buddy(PurpleBuddy* buddy);

		bool operator==(const Buddy& buddy) const;
		bool operator!=(const Buddy& buddy) const;

		bool isValid() const { return buddy != NULL; }

		string getName() const;
		string getAlias() const;
		bool isOnline() const;
		bool isAvailable() const;
		CacaImage getIcon() const;

		Account getAccount() const;

	};

};

#endif /* IM_BUDDY_H */
