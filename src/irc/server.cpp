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

#include "server.h"
#include "nick.h"

namespace irc {

Server::Server(string name, string _info)
	: Entity(name),
	  info(_info)
{}

void Server::addNick(Nick* n)
{
	users.push_back(n);
}

void Server::removeNick(Nick* n)
{
	for(vector<Nick*>::iterator it = users.begin(); it != users.end(); ++it)
		if (*it == n)
		{
			users.erase(it);
			return;
		}
}

unsigned Server::countNicks() const
{
	return users.size();
}

unsigned Server::countOnlineNicks() const
{
	unsigned i = 0;
	for(vector<Nick*>::const_iterator it = users.begin(); it != users.end(); ++it)
		if((*it)->isOnline())
			i++;
	return i;
}

RemoteServer::RemoteServer(IRC* _irc, im::Account _account)
	: Server(_account.getServername(),
	         _account.getProtocol().getName()),
	  account(_account),
	  irc(_irc)
{

}

}; /* namespace irc */
