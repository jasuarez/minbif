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
#include "../util.h"

namespace irc {

ChanUser::ChanUser(Channel* _chan, Nick* _nick, int _status)
	: Entity(_nick->getNickname()),
	  nick(_nick),
	  chan(_chan),
	  status(_status)
{
}

string ChanUser::getName() const
{
	return nick->getNickname();
}

string ChanUser::getLongName() const
{
	return nick->getLongName();
}

ChanUser::m2c_t ChanUser::m2c[] = {
	{ ChanUser::VOICE, 'v' },
	{ ChanUser::HALFOP,'h' },
	{ ChanUser::OP,    'o' },
};

ChanUser::mode_t ChanUser::c2mode(char c)
{
	size_t i;
	for(i=0; i < sizeof m2c / sizeof *m2c && m2c[i].c != c; ++i)
		;

	if(i < sizeof m2c / sizeof *m2c)
		return m2c[i].mode;
	else
		return (ChanUser::mode_t)0;
}

char ChanUser::mode2c(ChanUser::mode_t mode)
{
	size_t i;
	for(i=0; i < sizeof m2c / sizeof *m2c && m2c[i].mode != mode; ++i)
		;

	if(i < sizeof m2c / sizeof *m2c)
		return m2c[i].c;
	else
		return 0;
}

Message ChanUser::getModeMessage(bool add, int modes) const
{
	Message message(MSG_MODE);
	string modes_str = add ? "+" : "-";
	size_t i;

	if(!modes)
		modes = this->status;

	message.addArg(modes_str);
	for(i = 0; i < sizeof m2c / sizeof *m2c; ++i)
		if(m2c[i].mode & modes)
		{
			modes_str += m2c[i].c;
			message.addArg(getName());
		}
	message.setArg(0, modes_str);
	return message;
}

Channel::Channel(IRC* _irc, string name)
	: Entity(name),
	  irc(_irc)
{}

Channel::~Channel()
{
	for(vector<ChanUser*>::iterator it = users.begin(); it != users.end(); ++it)
	{
		(*it)->getNick()->removeChanUser(*it);
		delete *it;
	}
	users.clear();
}

void Channel::sendNames(Nick* nick) const
{
	string names;
	for(vector<ChanUser*>::const_iterator it = users.begin(); it != users.end(); ++it)
	{
		if(!names.empty())
			names += " ";
		if((*it)->hasStatus(ChanUser::OP))
			names += "@";
		else if((*it)->hasStatus(ChanUser::HALFOP))
			names += "%";
		else if((*it)->hasStatus(ChanUser::VOICE))
			names += "+";
		names += (*it)->getNick()->getNickname();

	}

	nick->send(Message(RPL_NAMREPLY).setSender(irc)
			           .setReceiver(nick)
				   .addArg("=")
				   .addArg(getName())
				   .addArg(names));
	nick->send(Message(RPL_ENDOFNAMES).setSender(irc)
			           .setReceiver(nick)
				   .addArg(getName())
				   .addArg("End of /NAMES list"));
}

ChanUser* Channel::addUser(Nick* nick, int status)
{
	ChanUser* chanuser = new ChanUser(this, nick, status);
	users.push_back(chanuser);

	string names;
	for(vector<ChanUser*>::iterator it = users.begin(); it != users.end(); ++it)
	{
		(*it)->getNick()->send(Message(MSG_JOIN).setSender(nick).setReceiver(this));
		if(status && (*it)->getNick() != nick)
		{
			Message m = chanuser->getModeMessage(true);
			m.setSender(irc);
			m.setReceiver(this);
			(*it)->getNick()->send(m);
		}
	}
	nick->send(Message(RPL_TOPIC).setSender(irc)
			             .setReceiver(nick)
				     .addArg(getName())
				     .addArg(getTopic()));

	sendNames(nick);
	return chanuser;
}

void Channel::delUser(Nick* nick, Message m)
{
	for(vector<ChanUser*>::iterator it = users.begin(); it != users.end(); )
		if((*it)->getNick() == nick)
		{
			delete *it;
			it = users.erase(it);
		}
		else
		{
			if(m.getCommand().empty() == false)
				(*it)->getNick()->send(m);
			++it;
		}
}

ChanUser* Channel::getChanUser(string nick) const
{
	for(vector<ChanUser*>::const_iterator it = users.begin(); it != users.end(); ++it)
		if((*it)->getNick()->getNickname() == nick)
			return *it;
	return NULL;
}

void Channel::broadcast(Message m, Nick* butone)
{
	for(vector<ChanUser*>::iterator it = users.begin(); it != users.end(); ++it)
		if(!butone || (*it)->getNick() != butone)
			(*it)->getNick()->send(m);
}

void Channel::m_mode(Nick* user, Message m)
{
	if(m.countArgs() == 0)
	{
		user->send(Message(RPL_CHANNELMODEIS).setSender(irc)
						     .setReceiver(user)
						     .addArg(getName())
						     .addArg("+"));
		user->send(Message(RPL_CREATIONTIME).setSender(irc)
						     .setReceiver(user)
						     .addArg(getName())
						     .addArg("1212313"));
		return;
	}
	vector<string> args = m.getArgs();
	vector<string>::iterator arg = args.begin();
	string modes = *arg++;
	bool add = true;

	FOREACH(string, modes, c)
	{
		switch(*c)
		{
			case '+':
				add = true;
				break;
			case '-':
				add = false;
				break;
			case 'b':
				if(arg == args.end())
					showBanList(user);
				else
				{
					string nick, ident, host, accid;
					im::Account account;

					accid = *arg++;
					if(accid.find(':') != string::npos)
					{
						host = stringtok(accid, ":");
						ident = stringtok(host, "@");
					}
					else
						ident = stringtok(accid, "@");
					if(accid.empty() || ident.find('!') != string::npos)
						nick = stringtok(ident, "!");

					if(add)
						processAddBan(user, nick, ident, host, accid);
					else
						processRemoveBan(user, nick, ident, host, accid);
				}
				break;
			default:
				break;
		}
	}

}

void Channel::setMode(const Entity* sender, int modes, ChanUser* chanuser)
{
	if(!modes) return;
	chanuser->setStatus(modes);
	Message m = chanuser->getModeMessage(true, modes);
	m.setSender(sender ? sender : irc);
	m.setReceiver(this);
	broadcast(m);
}

void Channel::delMode(const Entity* sender, int modes, ChanUser* chanuser)
{
	if(!modes) return;
	chanuser->delStatus(modes);
	Message m = chanuser->getModeMessage(false, modes);
	m.setSender(sender ? sender : irc);
	m.setReceiver(this);
	broadcast(m);

}

void Channel::setTopic(Entity* chanuser, string topic)
{
	this->topic = topic;
	broadcast(Message(MSG_TOPIC).setSender(chanuser ? chanuser : irc)
			            .setReceiver(this)
				    .addArg(topic));
}

}; /* namespace irc */
