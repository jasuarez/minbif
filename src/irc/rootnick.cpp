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

#include "rootnick.h"
#include "irc.h"
#include "user.h"
#include "../util.h"

static struct
{
	const char* cmd;
	void (RootNick::*func)(Message m);
	size_t minargs;
	const char* help;
} commands[] = {
	{ "help",      &RootNick::m_help,     0, "Display help" },
};

RootNick::RootNick(IRC* _irc)
	: Nick("root", "root", _irc->getServerName(), "User Manager"),
	  irc(_irc)
{}

RootNick::~RootNick()
{}

void RootNick::send(Message m)
{
	User* user = dynamic_cast<User*>(m.getSender());
	if(!user || user != irc->getUser())
		return;

	if(m.getCommand() != MSG_PRIVMSG)
		return;

	Nick *n = NULL;
	Channel *c = NULL;
	Message process_m;
	process_m.setReceiver(m.getReceiver());
	string text = m.getArg(0);
	string s;
	while((s = stringtok(text, " ")).empty() == false)
	{
		if(process_m.getCommand().empty())
			process_m.setCommand(s);
		else
			process_m.addArg(s);
	}
	if((n = dynamic_cast<Nick*>(m.getReceiver())))
	{
		if(n == this)
			processCommands(n, process_m);

	}
	else if((c = dynamic_cast<Channel*>(m.getReceiver())))
	{
		processCommands(c, process_m);
	}
}

void RootNick::processCommands(Entity* to, Message m)
{
	size_t i;
	for(i = 0;
	    i < (sizeof commands / sizeof *commands) &&
	    strcmp(commands[i].cmd, m.getCommand().c_str());
	    ++i)
		;

	if(i >= (sizeof commands / sizeof *commands))
		irc->getUser()->send(Message(MSG_PRIVMSG).setSender(this)
				             .setReceiver(to)
					     .addArg("Unknown command '" + m.getCommand() + "'. Please use 'help' command."));
	else if(m.countArgs() < commands[i].minargs)
		irc->getUser()->send(Message(MSG_PRIVMSG).setSender(this)
				             .setReceiver(to)
					     .addArg("Not enough parameters"));
	else
		(this->*commands[i].func)(m);

}

void RootNick::m_help(Message m)
{
	irc->getUser()->send(Message(MSG_PRIVMSG).setSender(this)
			                         .setReceiver(m.getReceiver())
						 .addArg("Available commands:"));

	size_t i;
	for(i = 0; i < (sizeof commands / sizeof *commands); ++i)
	{
		irc->getUser()->send(Message(MSG_PRIVMSG).setSender(this)
				                         .setReceiver(m.getReceiver())
							 .addArg(string("- ") + commands[i].cmd +
							         ": " + commands[i].help));
	}
}
