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
#include "core/callback.h"
#include "core/util.h"
#include "core/log.h"

namespace irc {

BuddyIcon::BuddyIcon(im::IM* _im, Server* server)
	: Nick(server, "buddyicon", "buddyicon", server->getName(), "DCC Send me a .png to set your icon"),
	  im(_im)
{
}

BuddyIcon::~BuddyIcon()
{}

void BuddyIcon::send(Message m)
{
	if(m.getCommand() == MSG_PRIVMSG && m.getReceiver() == this && m.countArgs() > 0)
	{
		string filename;
		uint32_t addr;
		uint16_t port;
		ssize_t size;

		if(DCCGet::parseDCCSEND(m.getArg(0), &filename, &addr, &port, &size))
		{
			string path = im->getBuddyIconPath();
			if(!check_write_file(path, filename))
			{
				b_log[W_ERR] << "Unable to write into the buddy icon directory '" << path << "': " << strerror(errno);
				return;
			}
			try
			{
				IRC* irc = dynamic_cast<IRC*>(getServer());
				filename = path + "/" + filename;
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
	if(filename)
		im->setBuddyIcon(filename);
	b_log[W_SNO|W_INFO] << "New icon set!";
	free(filename);
	return true;
}

}; /* namespace irc */
