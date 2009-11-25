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

#include <cassert>

#include "irc/message.h"
#include "irc/irc.h"
#include "irc/nick.h"
#include "irc/channel.h"
#include "core/util.h"
#include "core/entity.h"

namespace irc {

string Message::StoredEntity::getName() const
{
	return entity ? entity->getName() : name;
}

string Message::StoredEntity::getLongName() const
{
	return entity ? entity->getLongName() : name;
}

Message::Message(string _cmd)
	: cmd(_cmd)
{
}

Message::~Message()
{
}

string Message::format() const
{
	string buf;

	if(sender.isSet())
		buf = ":" + sender.getLongName() + " ";

	buf += cmd;
	if(receiver.isSet())
		buf += " " + receiver.getName();

	for(ArgVector::const_iterator it = args.begin(); it != args.end(); ++it)
	{
		buf += " ";
		if(it->isLong() || it->getStr().find(' ') != string::npos || it->getStr()[0] == ':')
			buf += ":";
		buf += it->getStr();
	}

	buf += "\r\n";

	return buf;
}

Message& Message::setCommand(string r)
{
	assert (r.empty() == false);

	cmd = r;
	return *this;
}

Message& Message::setSender(const Entity* entity)
{
	sender.setEntity(entity);
	return *this;
}

Message& Message::setSender(string n)
{
	sender.setName(n);
	return *this;
}

Message& Message::setReceiver(const Entity* entity)
{
	receiver.setEntity(entity);
	return *this;
}

Message& Message::setReceiver(string n)
{
	receiver.setName(n);
	return *this;
}

Message& Message::addArg(string s, bool is_long)
{
	if(!args.empty() && args.back().isLong())
		throw MalformedMessage();

	args.push_back(StoredArg(s, is_long));
	return *this;
}

Message& Message::setArg(size_t i, string s, bool is_long)
{
	if(i == args.size())
		return addArg(s);

	assert(i < args.size());

	if((i+1) < args.size() && args.back().isLong())
		throw MalformedMessage();

	args[i] = StoredArg(s, is_long);
	return *this;
}

string Message::getArg(size_t n) const
{
	assert(n < args.size());
	return args[n].getStr();
}

vector<string> Message::getArgsStr() const
{
	vector<string> v;
	for(ArgVector::const_iterator it = args.begin(); it != args.end(); ++it)
		v.push_back(it->getStr());
	return v;
}

Message Message::parse(string line)
{
	string s;
	Message m;
	while((s = stringtok(line, " ")).empty() == false)
	{
		if(m.getCommand().empty())
			m.setCommand(strupper(s));
		else if(s[0] == ':')
		{
			m.addArg(s.substr(1) + (line.empty() ? "" : " " + line), true);
			break;
		}
		else
			m.addArg(s);
	}
	return m;
}

void Message::rebuildWithQuotes()
{
	for(ArgVector::iterator s = args.begin(); s != args.end(); ++s)
	{
		if(s->getStr()[0] == '"' && s->getStr().find(' ') == string::npos)
		{
			ArgVector::iterator next = s;
			s->setStr(s->getStr().substr(1));

			while(next->getStr()[next->getStr().size()-1] != '"')
			{
				if(next == s)
					++next;
				else
					next = args.erase(next);

				if(next == args.end())
					break;

				s->setStr(" " + next->getStr());
			}
			if(s->getStr()[s->getStr().size()-1] == '"')
				s->setStr(s->getStr().substr(0, s->getStr().size()-1));
			if(next == args.end())
				break;
			else if(next != s)
				next = args.erase(next);
		}
	}
}

}; /* namespace irc */
