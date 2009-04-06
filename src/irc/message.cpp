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

#include "message.h"
#include "irc.h"
#include "nick.h"
#include "../util.h"

Message::Message(string _cmd)
	: cmd(_cmd)
{
}

Message::Message()
{
}

Message::~Message()
{
}

string Message::format() const
{
	string buf;

	if(!sender.empty())
		buf = ":" + sender;

	buf += " " + cmd;
	if(!receiver.empty())
		buf += " " + receiver;

	for(vector<string>::const_iterator it = args.begin(); it != args.end(); ++it)
	{
		buf += " ";
		if(it->find(' ') != string::npos)
			buf += ":";
		buf += *it;
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

Message& Message::setSender(const Nick* nick)
{
	assert (nick != NULL);

	sender = nick->getNickname()  + "!" +
		 nick->getIdentname() + "@" +
		 nick->getHostname();
	return *this;
}

Message& Message::setSender(const IRC* me)
{
	assert (me != NULL);

	sender = me->getServerName();
	return *this;
}

Message& Message::setReceiver(const Nick* nick)
{
	assert (nick != NULL);

	receiver = nick->getNickname();
	return *this;
}

Message& Message::setReceiver(string r)
{
	assert (r.empty() == false);

	receiver = r;
	return *this;
}

Message& Message::addArg(string s)
{
	if(!args.empty())
	{
		string last = args.back();
		if(last.find(' ') != string::npos)
			throw MalformedMessage();
	}

	args.push_back(s);
	return *this;
}

Message Message::parse(string s)
{
	string line = stringtok(s, "\r\n");
	Message m;
	while((s = stringtok(line, " ")).empty() == false)
	{
		if(m.getCommand().empty())
			m.setCommand(s);
		else if(s[0] == ':')
		{
			m.addArg(s.substr(1) + " " + line);
			break;
		}
		else
			m.addArg(s);
	}
	return m;
}
