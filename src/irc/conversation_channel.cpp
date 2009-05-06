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

#include "conversation_channel.h"
#include "chat_buddy.h"
#include "nick.h"
#include "user.h"
#include "irc.h"

namespace irc {

ConversationChannel::ConversationChannel(IRC* irc, im::Conversation _conv)
	: Channel(irc, _conv.getChanName()),
	  conv(_conv),
	  upserver(NULL)
{

	upserver = dynamic_cast<RemoteServer*>(irc->getServer(conv.getAccount().getServername()));
}


void ConversationChannel::showBanList(Nick* to)
{
	to->send(Message(RPL_ENDOFBANLIST).setSender(irc)
			                  .setReceiver(to)
					  .addArg(getName())
					  .addArg("End of Channel Ban List"));
}

void ConversationChannel::processAddBan(Nick* from, string nick, string ident, string host, string accid)
{

}

void ConversationChannel::processRemoveBan(Nick* from, string nick, string ident, string host, string accid)
{

}

void ConversationChannel::addBuddy(im::ChatBuddy cbuddy)
{
	ChanUser* cul;
	if(cbuddy.isMe())
		cul = irc->getUser()->getChanUser(this);
	else
	{
		map<im::ChatBuddy, ChanUser*>::iterator it = cbuddies.find(cbuddy);
		if(it == cbuddies.end())
		{
			ChatBuddy* n = new ChatBuddy(upserver, cbuddy);
			while(irc->getNick(n->getNickname()))
				n->setNickname(n->getNickname() + "_");

			irc->addNick(n);
			cul = n->join(this);
		}
		else
			cul = it->second;
	}

	cbuddies[cbuddy] = cul;
}

ChanUser* ConversationChannel::getChanUser(string nick) const
{
	map<im::ChatBuddy, ChanUser*>::const_iterator it;
	for(it = cbuddies.begin(); it != cbuddies.end(); ++it)
		if(it->first.getName() == nick)
			return it->second;
	return NULL;
}

void ConversationChannel::broadcast(Message m, Nick* butone)
{
	if(m.getCommand() == MSG_PRIVMSG && m.getSender() == irc->getUser())
	{
		conv.sendMessage(m.getArg(0));
	}
	else
		Channel::broadcast(m, butone);
}

}; /* namespace irc */