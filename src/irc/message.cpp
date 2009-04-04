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

	if(!sender.empty())
		buf = ":" + sender;

	buf += " " + cmd;
	if(!receiver.empty())
		buf += " " + receiver;

	for(vector<string>::const_iterator it = args.begin(); it != args.end(); ++it)
		buf += " " + *it;

	buf += "\r\n";

	return buf;
}

Message& Message::setSender(Nick* nick)
{
	assert (nick != NULL);

	sender = nick->getNickname() + "!" +
		 nick->getIdent()    + "@" +
		 nick->getHostname();
	return *this;
}

Message& Message::setSender(IRC* me)
{
	assert (me != NULL);

	sender = me->getServerName();
	return *this;
}

Message& Message::setReceiver(Nick* nick)
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
	if(s.find(' ') != string::npos)
		s = ":" + s;

	args.push_back(s);
	return *this;
}
