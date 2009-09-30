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
#include <cerrno>

#include "irc/buddy_icon.h"
#include "irc/irc.h"
#include "irc/dcc.h"
#include "im/im.h"
#include "callback.h"
#include "../util.h"
#include "../log.h"

namespace irc {

BuddyIcon::BuddyIcon(im::IM* _im, Server* server, string nickname, string username, string realname)
	: Nick(server, nickname, username, realname),
	  im(_im)
{
}

BuddyIcon::~BuddyIcon()
{}

void BuddyIcon::send(Message m)
{
	if(m.getCommand() == MSG_PRIVMSG && m.countArgs() > 0 && m.getArg(0)[0] == '\1')
	{
		IRC* irc = dynamic_cast<IRC*>(getServer());

		string line = m.getArg(0);
		string word;
		Message args;

		/* Remove \1 chars. */
		line = line.substr(1, line.size()-2);

		while((word = stringtok(line, " ")).empty() == false)
			args.addArg(word);
		args.rebuildWithQuotes();

		if(args.countArgs() == 6 && args.getArg(0) == "DCC" && args.getArg(1) == "SEND")
		{
			string filename = im->getBuddyIconPath() + "/" + args.getArg(2);
			uint32_t addr = s2t<uint32_t>(args.getArg(3));
			uint16_t port = s2t<uint16_t>(args.getArg(4));
			ssize_t size = s2t<ssize_t>(args.getArg(5));

			if(!check_write_file(im->getBuddyIconPath(), args.getArg(2)))
			{
				b_log[W_ERR] << "Unable to write into the buddy icon directory '" << filename << "': " << strerror(errno);
				return;
			}
			try
			{
				irc->createDCCGet(this, filename, addr, port, size, new CallBack<BuddyIcon>(this, &BuddyIcon::receivedIcon, strdup(filename.c_str())));
			}
			catch(const DCCGetError&)
			{
			}
		}
	}
}

bool BuddyIcon::receivedIcon(void* data)
{
	char* filename = static_cast<char*>(data);
	b_log[W_ERR] << "New icon set!";
	if(filename)
		im->setBuddyIcon(filename);
	free(filename);
	return true;
}

}; /* namespace irc */
