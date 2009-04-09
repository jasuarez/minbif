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

#include "rootnick.h"
#include "irc.h"

#if 0
static struct
{
	const char* cmd;
	void (RootNick::*func)(vector<string> args);
	size_t minargs;
	const char* help;
} commands[] = {
	{ "help",      &RootNick::m_help,     0, "Display help" },
};
#endif

RootNick::RootNick(IRC* _irc)
	: Nick("root", "root", _irc->getServerName(), "User Manager"),
	  irc(_irc)
{}

RootNick::~RootNick()
{}

void RootNick::send(Message m)
{

}
