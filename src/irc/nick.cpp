/*
 * Copyright(C) 2009-2010 Romain Bignon
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
#include <cassert>
#include <algorithm>

#include "irc/nick.h"
#include "irc/server.h"
#include "irc/channel.h"
#include "core/caca_image.h"
#include "core/util.h"

namespace irc {

const char *Nick::nick_lc_chars = "0123456789abcdefghijklmnopqrstuvwxyz{}^`-_|";
const char *Nick::nick_uc_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ[]~`-_\\";
const char *Nick::UMODES = "o";

Nick::Nick(Server* _server, string nickname, string _identname, string _hostname, string _realname)
	: Entity(nickname),
	  server(_server),
	  flags(0)
{
	assert(server);
	setIdentname(_identname);
	setHostname(_hostname);
	setRealname(_realname);
}

Nick::~Nick()
{
	quit("*.net *.split");
}

bool Nick::isValidNickname(const string& nick)
{
	/* Empty/long nicks are not allowed, nor numbers at [0] */
	if(nick.empty() || isdigit(nick[0]) || nick.size() > Nick::MAX_LENGTH)
		return false;

	for(string::const_iterator i = nick.begin(); i != nick.end(); ++i)
		if(!strchr(Nick::nick_lc_chars, *i) && !strchr(Nick::nick_uc_chars, *i))
			return false;

	return true;

}

string Nick::nickize(const string& n)
{
	string nick;

	/* Transliterate string. */
	GIConv ic = g_iconv_open("ASCII//TRANSLIT", "UTF-8");
	gchar* conv = g_convert_with_iconv(n.c_str(), n.size(), ic, NULL, NULL, NULL);

	for(const char* c = conv ? conv : n.c_str(); *c; ++c)
		if (*c == ' ')
			nick += "_";
		else if (strchr(nick_lc_chars, *c) || strchr(nick_uc_chars, *c))
			nick += *c;

	g_iconv_close(ic);
	g_free(conv);

	if(isdigit(nick[0]))
		nick = "_" + nick;

	if(nick.size() > MAX_LENGTH)
		nick = nick.substr(0, MAX_LENGTH);

	/* Empty nicknames are not allowed. */
	if (nick.empty())
		nick = "Invalid";

	return nick;
}

void Nick::setNickname(string n)
{
	setName(n);
}

void Nick::setIdentname(string n)
{
	for(string::iterator i = n.begin(); i != n.end(); ++i)
		if(*i == ' ')
			*i = '_';
	identname = n;
}

void Nick::setHostname(string n)
{
	for(string::iterator i = n.begin(); i != n.end(); ++i)
		if(*i == ' ')
			*i = '.';
	hostname = n;
}

string Nick::getLongName() const
{
	return getNickname()  + "!" +
	       getIdentname() + "@" +
	       getHostname();
}

CacaImage Nick::getIcon() const
{
	return CacaImage();
}

vector<ChanUser*> Nick::getChannels() const
{
	return channels;
}

bool Nick::isOn(const Channel* chan) const
{
	for(vector<ChanUser*>::const_iterator it = channels.begin(); it != channels.end(); ++it)
		if((*it)->getChannel() == chan)
			return true;
	return false;
}

ChanUser* Nick::getChanUser(const Channel* chan) const
{
	for(vector<ChanUser*>::const_iterator it = channels.begin(); it != channels.end(); ++it)
		if((*it)->getChannel() == chan)
			return *it;
	return NULL;
}

ChanUser* Nick::join(Channel* chan, int status)
{
	ChanUser* chanuser;
	if((chanuser = getChanUser(chan)))
	{
		int last_status = chanuser->getStatus();
		if(last_status != status)
		{
			chan->delMode(NULL, last_status & ~status, chanuser);
			chan->setMode(NULL, status & ~last_status, chanuser);
		}
		return chanuser;
	}

	chanuser = chan->addUser(this, status);
	channels.push_back(chanuser);
	return chanuser;
}

void Nick::part(Channel* chan, string message)
{
	Message m = Message(MSG_PART).setSender(this)
				     .setReceiver(chan)
				     .addArg(message);
	for(vector<ChanUser*>::iterator it = channels.begin(); it != channels.end();)
		if((*it)->getChannel() == chan)
		{
			ChanUser* chanuser = *it;
			it = channels.erase(it);
			send(m);
			chanuser->getChannel()->delUser(this, m);
			break;
		}
		else
			++it;
}

void Nick::removeChanUser(ChanUser* chanuser)
{
	for(vector<ChanUser*>::iterator it = channels.begin(); it != channels.end();)
		if(*it == chanuser)
			it = channels.erase(it);
		else
			++it;
}

void Nick::kicked(Channel* chan, ChanUser* from, string message)
{
	for(vector<ChanUser*>::iterator it = channels.begin(); it != channels.end();)
		if((*it)->getChannel() == chan)
		{
			ChanUser* cu = *it;
			it = channels.erase(it);
			cu->getChannel()->delUser(this, Message(MSG_KICK).setSender(from)
					                                 .setReceiver(cu->getChannel())
									 .addArg(getNickname())
									 .addArg(message));
		}
		else
			++it;
}

void Nick::quit(string text)
{
	Message m = Message(MSG_QUIT).setSender(this)
		                     .addArg(text);
	vector<Nick*> sended;

	for(vector<ChanUser*>::iterator it = channels.begin(); it != channels.end();)
	{
		vector<ChanUser*> users = (*it)->getChannel()->getChanUsers();
		FOREACH(vector<ChanUser*>, users, u)
		{
			Nick* n = (*u)->getNick();
			if(std::find(sended.begin(), sended.end(), n) == sended.end())
			{
				sended.push_back(n);
				n->send(m);
			}
		}
		(*it)->getChannel()->delUser(this);
		it = channels.erase(it);
	}
}

void Nick::privmsg(Channel* chan, string msg)
{
	string tmp;
	while((tmp = stringtok(msg, "\n\r")).empty() == false)
		chan->broadcast(Message(MSG_PRIVMSG).setSender(this)
						    .setReceiver(chan)
						    .addArg(tmp),
				this);
}

void Nick::privmsg(Nick* nick, string msg)
{
	string tmp;
	while((tmp = stringtok(msg, "\n\r")).empty() == false)
		nick->send(Message(MSG_PRIVMSG).setSender(this)
					       .setReceiver(nick)
					       .addArg(tmp));
}

void Nick::m_mode(Nick* user, Message m)
{
	user->send(Message(ERR_NOPRIVILEGES).setSender(getServer())
					    .setReceiver(user)
					    .addArg("Permission Denied: Insufficient privileges"));
}

}; /* namespace irc */
