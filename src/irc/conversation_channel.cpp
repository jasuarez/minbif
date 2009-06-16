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

#include "conversation_channel.h"
#include "chat_buddy.h"
#include "nick.h"
#include "user.h"
#include "irc.h"
#include "../log.h"

namespace irc {

ConversationChannel::ConversationChannel(IRC* irc, const im::Conversation& _conv)
	: Channel(irc, _conv.getChanName()),
	  conv(_conv),
	  upserver(NULL)
{

	upserver = dynamic_cast<RemoteServer*>(irc->getServer(conv.getAccount().getServername()));
}

ConversationChannel::~ConversationChannel()
{
	map<im::ChatBuddy, ChanUser*>::iterator it;
	for(it = cbuddies.begin(); it != cbuddies.end(); ++it)
	{
		irc::ChatBuddy* cb = dynamic_cast<irc::ChatBuddy*>(it->second->getNick());

		if(cb)
		{
			/* Call the Channel function to not erase chatbuddy from cbuddies,
			 * because I iterate on.
			 * I know that Channel destructor will also remove chanuser from list,
			 * but it may also call some Nick methods, as the object is deleted
			 * by IRC::removeNick()...
			 */
			Channel::delUser(cb);
			cb->removeChanUser(it->second);
			irc->removeNick(cb->getNickname());
		}
	}
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

void ConversationChannel::addBuddy(im::ChatBuddy cbuddy, int status)
{
	ChanUser* cul;
	if(cbuddy.isMe())
		cul = irc->getUser()->join(this, status);
	else
	{
		map<im::ChatBuddy, ChanUser*>::iterator it = cbuddies.find(cbuddy);
		if(it == cbuddies.end())
		{
			ChatBuddy* n = new ChatBuddy(upserver, cbuddy);
			while(irc->getNick(n->getNickname()))
				n->setNickname(n->getNickname() + "_");

			irc->addNick(n);
			cul = n->join(this, status);
		}
		else
			cul = it->second;
	}

	cbuddies[cbuddy] = cul;
}

void ConversationChannel::delUser(Nick* nick, Message message)
{
	map<im::ChatBuddy, ChanUser*>::iterator it;
	for(it = cbuddies.begin(); it != cbuddies.end() && it->second->getNick() != nick; ++it)
		;

	if(it != cbuddies.end())
		cbuddies.erase(it);

	Channel::delUser(nick, message);

	irc::ChatBuddy* cb = dynamic_cast<irc::ChatBuddy*>(nick);
	if(cb)
		irc->removeNick(cb->getNickname());
	else if(nick == irc->getUser())
		conv.leave();
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

string ConversationChannel::getTopic() const
{
	return conv.getChanTopic();
}

void ConversationChannel::invite(const string& buddy, string message)
{
	conv.invite(buddy, message);
}

}; /* namespace irc */
