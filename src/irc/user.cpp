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

#include <cstdio>
#include "user.h"
#include "server.h"

namespace irc {

User::User(SockWrapper* _sockw, Server* server, string nickname, string identname, string hostname, string realname)
	: Nick(server, nickname, identname, hostname, realname),
	  sockw(_sockw)
{
}

User::~User()
{
}

void User::send(Message msg)
{
	if (sockw)
		sockw->Write(msg.format());
}

void User::setLastReadNow()
{
	last_read = time(NULL);
}

void User::m_mode(Nick* user, Message m)
{
	if(m.countArgs() == 0)
	{
		user->send(Message(RPL_UMODEIS).setSender(getServer())
				               .setReceiver(user)
					       .addArg(getModes()));
		return;
	}

	/* TODO set personal modes. */
}

string User::getModes() const
{
	string modes = "+";
	if(hasFlag(OPER))
		modes += "o";

	return modes;
}

}; /* namespace irc */
