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

#ifndef IRC_IRC_H
#define IRC_IRC_H

#include <string>
#include <map>
#include <exception>

#include "message.h"
#include "server.h"

using std::string;
using std::map;

class IRCAuthError : public std::exception {};

class User;
class Nick;
class Channel;
class _CallBack;
class ServerPoll;
class IM;

class IRC : public Server
{
	ServerPoll* poll;
	int fd;
	int read_id;
	_CallBack *read_cb;
	_CallBack *ping_cb;
	User* user;
	IM* im;

	map<string, Nick*> users;
	map<string, Channel*> channels;
	map<string, Server*> servers;

public:

	IRC(ServerPoll* poll, int fd, string hostname, string command_chan, unsigned ping_freq);
	~IRC();

	User* getUser() const { return user; }

	void sendWelcome();
	void quit(string reason = "");

	void addChannel(Channel* chan);
	Channel* getChannel(string channame) const;
	void removeChannel(string channame);

	void addNick(Nick* nick);
	Nick* getNick(string nick) const;
	void removeNick(string nick);

	bool ping(void*);

	bool readIO(void*);
	void m_nick(Message m);
	void m_user(Message m);
	void m_pass(Message m);
	void m_quit(Message m);
	void m_ping(Message m);
	void m_pong(Message m);
	void m_whois(Message m);
	void m_whowas(Message m);
	void m_version(Message m);
	void m_privmsg(Message m);
};

#endif /* IRC_IRC_H */
