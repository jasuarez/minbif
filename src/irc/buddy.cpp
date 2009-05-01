/*
 * Bitlbee v2 - IRC instant messaging gateway
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

#include "irc/buddy.h"
#include "irc/server.h"
#include "irc/channel.h"
#include "im/account.h"
#include "../util.h"

namespace irc {

Buddy::Buddy(Server* server, im::Buddy _buddy)
	: Nick(server, "","","",_buddy.getRealName()),
	  im_buddy(_buddy)
{
	string hostname = im_buddy.getName();
	string identname = stringtok(hostname, "@");
	string nickname = im_buddy.getAlias();
	if(nickname.find('@') != string::npos || nickname.find(' ') != string::npos)
		nickname = nickize(identname);
	else
		nickname = nickize(nickname);
	if(hostname.empty())
		hostname = im_buddy.getAccount().getID();

	setNickname(nickname);
	setIdentname(identname);
	setHostname(hostname);
}

Buddy::~Buddy()
{}

string Buddy::nickize(const string& n)
{
	string nick;
	for(string::const_iterator c = n.begin(); c != n.end(); ++c)
		if(strchr(nick_lc_chars, *c) || strchr(nick_uc_chars, *c))
			nick += *c;
	if(isdigit(nick[0]))
		nick = "_" + nick;

	if(nick.size() > MAX_LENGTH)
		nick = nick.substr(0, MAX_LENGTH);

	return nick;
}

void Buddy::send(Message m)
{
	if(m.getCommand() == MSG_PRIVMSG)
	{
		string text = m.getArg(0);
		const Channel* chan = dynamic_cast<const Channel*>(m.getReceiver());

		if(m.getReceiver() == this || (chan && chan->isStatusChannel() && text.find(getNickname() + ": ") == 0))
		{
			if(chan && chan->isStatusChannel())
				stringtok(text, " ");
			if(!conv.isValid())
			{
				conv = im::Conversation(im_buddy.getAccount(), im_buddy);
				conv.present();
			}
			conv.sendMessage(text);
		}
	}
}

string Buddy::getRealName() const
{
	return im_buddy.getRealName();
}

string Buddy::getAwayMessage() const
{
	if(im_buddy.isOnline() == false)
		return "User is offline";
	return Nick::getAwayMessage();
}

bool Buddy::isAway() const
{
	return im_buddy.isOnline() == false || im_buddy.isAvailable() == false;
}

bool Buddy::isOnline() const
{
	return im_buddy.isOnline();
}

CacaImage Buddy::getIcon() const
{
	return im_buddy.getIcon();
}

}; /* namespace buddy */
