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

#include "irc/buddy.h"
#include "../util.h"

namespace irc {

Buddy::Buddy(Server* server, im::Buddy _buddy)
	: Nick(server, "","","",_buddy.getName()),
	  im_buddy(_buddy)
{
	string hostname = im_buddy.getName();
	string nickname = stringtok(hostname, "@");
	string identname = nickname;
	setNickname(nickname);
	setIdentname(identname);
	setHostname(hostname);
}

Buddy::~Buddy()
{}

void Buddy::send(Message m)
{}

string Buddy::getAwayMessage() const
{
	if(im_buddy.isOnline() == false)
		return "User is offline";
	return Nick::getAwayMessage();
}

bool Buddy::isAway() const
{
	return im_buddy.isOnline() == false || Nick::isAway();
}

}; /* namespace buddy */
