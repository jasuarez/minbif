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

#ifndef IRC_BUDDY_H
#define IRC_BUDDY_H

#include "nick.h"
#include "im/buddy.h"

namespace irc
{
	class Buddy : public Nick
	{
		im::Buddy im_buddy;

	public:

		Buddy(Server* server, im::Buddy buddy);
		~Buddy();

		virtual void send(Message m);

	};

}; /* namespace irc */

#endif /* IRC_BUDDY_H */
