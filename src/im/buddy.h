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

#ifndef IM_BUDDY_H
#define IM_BUDDY_H

#include <libpurple/purple.h>
#include <string>

#include "caca_image.h"

namespace im
{
	using std::string;

	class Account;

	/** This class represents a buddy.
	 *
	 * This class only interfaces between the minbif code
	 * and a libpurple account object.
	 */
	class Buddy
	{
		PurpleBuddy* buddy;

		static void* getHandler();
		static PurpleBlistUiOps blist_ui_ops;
		static void update_node(PurpleBuddyList *list, PurpleBlistNode *node);
		static void removed_node(PurpleBuddyList *list, PurpleBlistNode *node);

	public:

		/** Initialization of libpurple buddies' stuffs. */
		static void init();

		/** Empty constructor */
		Buddy();

		/** Create a Buddy instance
		 *
		 * @param buddy  the libpurple's buddy instance.
		 */
		Buddy(PurpleBuddy* buddy);

		/** Comparaisons */
		bool operator==(const Buddy& buddy) const;
		bool operator!=(const Buddy& buddy) const;

		bool isValid() const { return buddy != NULL; }

		/** Get username of buddy */
		string getName() const;

		/** Get minbif alias */
		string getAlias() const;

		/** Get IM real name */
		string getRealName() const;

		bool isOnline() const;
		bool isAvailable() const;
		string getStatus() const;

		/** Get buddy's icon as a colored ASCII-art picture.
		 *
		 * @return  an instance of CacaImage
		 */
		CacaImage getIcon() const;

		PurpleGroup* getPurpleGroup() const;
		PurpleBuddy* getPurpleBuddy() const { return buddy; }

		/** Get account this buddy is from. */
		Account getAccount() const;

	};

};

#endif /* IM_BUDDY_H */
