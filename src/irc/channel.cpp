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

#include "channel.h"
#include "nick.h"
#include "message.h"
#include "irc.h"

ChanUser::ChanUser(Nick* _nick, int _status)
	: nick(_nick),
	  status(_status)
{
}

Channel::Channel(IRC* _irc, string _name)
	: irc(_irc),
	  name(_name)
{}

Channel::~Channel()
{
}


void Channel::addUser(Nick* nick, int status)
{
	users.push_back(ChanUser(nick, status));

	string names;
	for(vector<ChanUser>::iterator it = users.begin(); it != users.end(); ++it)
	{
		it->getNick()->send(Message(MSG_JOIN).setSender(nick).setReceiver(this));
		if(status && it->getNick() != nick)
		{
			string modes = "+";
			Message m(MSG_MODE);
			m.setSender(irc);
			m.setReceiver(this);
			m.addArg("");
			if(status & ChanUser::OP)
			{
				modes += "o";
				m.addArg(nick->getNickname());
			}
			if(status & ChanUser::VOICE)
			{
				modes += "v";
				m.addArg(nick->getNickname());
			}
			m.setArg(0, modes);
			it->getNick()->send(m);
		}

		if(!names.empty())
			names += " ";
		if(it->hasStatus(ChanUser::OP))
			names += "@";
		else if(it->hasStatus(ChanUser::VOICE))
			names += "+";
		names += it->getNick()->getNickname();

	}
	nick->send(Message(RPL_353).setSender(irc)
			           .setReceiver(nick)
				   .addArg("=")
				   .addArg(getName())
				   .addArg(names));
	nick->send(Message(RPL_366).setSender(irc)
			           .setReceiver(nick)
				   .addArg(getName())
				   .addArg("End of /NAMES list"));
}

void Channel::broadcast(Message m, Nick* butone)
{
	for(vector<ChanUser>::iterator it = users.begin(); it != users.end(); ++it)
		if(!butone || it->getNick() != butone)
			it->getNick()->send(m);
}
