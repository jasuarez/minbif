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

#include <cstring>

#include "irc/unknown_buddy.h"
#include "im/account.h"
#include "../util.h"

namespace irc {

UnknownBuddy::UnknownBuddy(Server* server, im::Conversation _conv)
	: ConvNick(server, "","","",""),
	  conv(_conv)
{
	string hostname = conv.getName();
	string identname = stringtok(hostname, "@");
	string nickname = conv.getName();
	if(nickname.find('@') != string::npos || nickname.find(' ') != string::npos)
		nickname = nickize(identname);
	else
		nickname = nickize(nickname);
	if(hostname.empty())
		hostname = conv.getAccount().getID();
	else
		hostname += ":" + conv.getAccount().getID();

	setNickname(nickname);
	setIdentname(identname);
	setHostname(hostname);
}

UnknownBuddy::~UnknownBuddy()
{
	if(conv.isValid() && conv.getNick() == this)
		conv.setNick(NULL, false);
}

void UnknownBuddy::send(Message m)
{
	if(m.getCommand() == MSG_PRIVMSG)
	{
		string text = m.getArg(0);
		if(m.getReceiver() == this && conv.isValid())
			conv.sendMessage(text);
	}
}

string UnknownBuddy::getRealName() const
{
	return conv.getName();
}

string UnknownBuddy::getAwayMessage() const
{
	/* TODO */
	return "";
}

bool UnknownBuddy::isAway() const
{
	/* TODO */
	return false;
}

}; /* namespace irc */
