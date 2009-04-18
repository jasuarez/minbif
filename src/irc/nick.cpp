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

#include <cstring>
#include <cassert>

#include "nick.h"
#include "channel.h"

namespace irc {

const char *Nick::nick_lc_chars = "0123456789abcdefghijklmnopqrstuvwxyz{}^`-_|";
const char *Nick::nick_uc_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ[]~`-_\\";

Nick::Nick(Server* _server, string nickname, string _identname, string _hostname, string _realname)
	: Entity(nickname),
	  identname(_identname),
	  hostname(_hostname),
	  realname(_realname),
	  server(_server),
	  flags(0)
{
	assert(server);
}

Nick::~Nick()
{
	for(vector<ChanUser>::iterator it = channels.begin(); it != channels.end(); ++it)
		it->getChannel()->delUser(this,
				          Message(MSG_QUIT).setSender(this)
						           .addArg("*.net *.split"));
}

bool Nick::isValidNickname(const string& nick)
{
	/* Empty/long nicks are not allowed, nor numbers at [0] */
	if(nick.empty() || isdigit(nick[0]) || nick.size() > Nick::MAX_LENGTH)
		return false;

	for(string::const_iterator i = nick.begin(); i != nick.end(); ++i)
		if(!strchr(Nick::nick_lc_chars, *i) && !strchr(Nick::nick_uc_chars, *i))
			return false;

	return true;

}

string Nick::getLongName() const
{
	return getNickname()  + "!" +
	       getIdentname() + "@" +
	       getHostname();
}

vector<ChanUser> Nick::getChannels() const
{
	return channels;
}

bool Nick::isOn(const Channel* chan) const
{
	for(vector<ChanUser>::const_iterator it = channels.begin(); it != channels.end(); ++it)
		if(it->getChannel() == chan)
			return true;
	return false;
}

void Nick::join(Channel* chan, int status)
{
	channels.push_back(chan->addUser(this, status));
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

}; /* namespace irc */
