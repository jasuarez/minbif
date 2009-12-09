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

#include "irc/buddy.h"
#include "irc/server.h"
#include "irc/channel.h"
#include "irc/dcc.h"
#include "irc/irc.h"
#include "im/im.h"
#include "im/account.h"
#include "core/callback.h"
#include "core/log.h"
#include "core/util.h"
#include "core/config.h"

namespace irc {

Buddy::Buddy(Server* server, im::Buddy _buddy)
	: ConvNick(server, "","","",_buddy.getRealName()),
	  im_buddy(_buddy)
{
	string hostname = im_buddy.getName();
	string identname = stringtok(hostname, "@");
	string nickname = im_buddy.getAlias();
	if(nickname.find('@') != string::npos || nickname.find(' ') != string::npos)
		nickname = nickize(identname);
	else
		nickname = nickize(nickname);
	if(hostname.empty())
		hostname = im_buddy.getAccount().getID();
	else
		hostname += ":" + im_buddy.getAccount().getID();

	setNickname(nickname);
	setIdentname(identname);
	setHostname(hostname);

	im_buddy.setNick(this);
}

Buddy::~Buddy()
{
	if(conv.isValid() && conv.getNick() == this)
		conv.setNick(NULL);
	if(im_buddy.isValid())
		im_buddy.setNick(NULL);
}

void Buddy::send(Message m)
{
	if(m.getCommand() == MSG_PRIVMSG)
	{
		string text = m.getArg(0);
		const Channel* chan = dynamic_cast<const Channel*>(m.getReceiver());

		if(m.getReceiver() == this || (chan && chan->isStatusChannel() && text.find(getNickname() + ": ") == 0))
		{
			if(chan && chan->isStatusChannel())
				stringtok(text, " ");

			/* Check if this is a DCC SEND message. */
			if(process_dcc_get(text))
				return;

			if(!conv.isValid())
			{
				conv = im::Conversation(im_buddy.getAccount(), im_buddy);
				conv.present();
				conv.setNick(this);
			}
			conv.sendMessage(text);
		}
	}
}

bool Buddy::process_dcc_get(const string& text)
{
	if(conf.GetSection("file_transfers")->GetItem("enabled")->Boolean() == false)
	{
		b_log[W_ERR] << "File transfers are disabled on this server.";
		return true;
	}

	string filename;
	uint32_t addr;
	uint16_t port;
	ssize_t size;

	if(DCCGet::parseDCCSEND(text, &filename, &addr, &port, &size))
	{
		RemoteServer* rm = dynamic_cast<RemoteServer*>(getServer());
		if(!rm)
			return true;

		string path = rm->getIRC()->getIM()->getUserPath() + "/upload/";
		if(!check_write_file(path, filename))
		{
			b_log[W_ERR] << "Unable to write into the upload directory '" << path << "': " << strerror(errno);
			return true;
		}
		try
		{
			filename = path + "/" + filename;
			rm->getIRC()->createDCCGet(this, filename, addr, port, size, new CallBack<Buddy>(this, &Buddy::received_file, strdup(filename.c_str())));

		}
		catch(const DCCGetError&)
		{
		}

		return true;
	}

	return false;
}

bool Buddy::received_file(void* data)
{
	char* filename = static_cast<char*>(data);
	im_buddy.sendFile(filename);
	free(filename);
	return true;
}

string Buddy::getRealName() const
{
	return im_buddy.getRealName() + " [Group: " + im_buddy.getGroupName() + "]";
}

string Buddy::getAwayMessage() const
{
	if(im_buddy.isOnline() == false)
		return "User is offline";
	return im_buddy.getStatus();
}

bool Buddy::isAway() const
{
	return im_buddy.isOnline() == false || im_buddy.isAvailable() == false;
}

bool Buddy::isOnline() const
{
	return im_buddy.isOnline();
}

CacaImage Buddy::getIcon() const
{
	return im_buddy.getIcon();
}

string Buddy::getIconPath() const
{
	return im_buddy.getIconPath();
}

bool Buddy::retrieveInfo() const
{
	im_buddy.retrieveInfo();
	return true;
}

int Buddy::sendCommand(const string& cmd)
{
	return conv.sendCommand(cmd);
}

}; /* namespace irc */
