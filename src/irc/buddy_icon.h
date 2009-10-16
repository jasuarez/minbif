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

#ifndef IRC_BUDDY_ICON_H
#define IRC_BUDDY_ICON_H

#include "nick.h"

namespace im
{
	class IM;
};

namespace irc
{

	/** This class represents a buddy on IRC */
	class BuddyIcon : public Nick
	{
		im::IM* im;
		bool receivedIcon(void* data);
	public:

		/** Build buddy object
		 *
		 * @param im  IM instance
		 * @param server  up-server
		 */
		BuddyIcon(im::IM* im, Server* server);
		~BuddyIcon();

		/** Implementation of the message routing to this buddy */
		virtual void send(Message m);
	};

}; /* namespace irc */

#endif /* IRC_BUDDY_ICON_H */
