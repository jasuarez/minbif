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

#include "nick.h"

Nick::Nick(string nickname, string _identname, string _hostname, string _realname)
	: Entity(nickname),
	  identname(_identname),
	  hostname(_hostname),
	  realname(_realname),
	  flags(0)
{
}

Nick::~Nick()
{
}

string Nick::getLongName() const
{
	return getNickname()  + "!" +
	       getIdentname() + "@" +
	       getHostname();
}

void Nick::join(Channel* chan, int status)
{
	chan->addUser(this, status);
}

void Nick::privmsg(Channel* chan, string msg)
{
	chan->broadcast(Message(MSG_PRIVMSG).setSender(this)
			                    .setReceiver(chan)
					    .addArg(msg),
			this);
}

void Nick::privmsg(Nick* nick, string msg)
{
	nick->send(Message(MSG_PRIVMSG).setSender(this)
			               .setReceiver(nick)
				       .addArg(msg));
}
