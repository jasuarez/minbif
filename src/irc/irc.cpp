/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon, Marc Dequènes (Duck)
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

#include <sys/socket.h>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <fnmatch.h>

#include "core/log.h"
#include "core/util.h"
#include "core/version.h"
#include "server_poll/poll.h"
#include "irc/irc.h"
#include "irc/buddy.h"
#include "irc/dcc.h"
#include "irc/user.h"
#include "irc/channel.h"

namespace irc {

IRC::command_t IRC::commands[] = {
	{ MSG_NICK,    &IRC::m_nick,    0, 0, 0 },
	{ MSG_USER,    &IRC::m_user,    4, 0, 0 },
	{ MSG_PASS,    &IRC::m_pass,    1, 0, 0 },
	{ MSG_QUIT,    &IRC::m_quit,    0, 0, 0 },
	{ MSG_CMD,     &IRC::m_cmd,     2, 0, Nick::REGISTERED },
	{ MSG_PRIVMSG, &IRC::m_privmsg, 2, 0, Nick::REGISTERED },
	{ MSG_PING,    &IRC::m_ping,    0, 0, Nick::REGISTERED },
	{ MSG_PONG,    &IRC::m_pong,    1, 0, Nick::REGISTERED },
	{ MSG_VERSION, &IRC::m_version, 0, 0, Nick::REGISTERED },
	{ MSG_INFO,    &IRC::m_info,    0, 0, Nick::REGISTERED },
	{ MSG_WHO,     &IRC::m_who,     0, 0, Nick::REGISTERED },
	{ MSG_WHOIS,   &IRC::m_whois,   1, 0, Nick::REGISTERED },
	{ MSG_WHOWAS,  &IRC::m_whowas,  1, 0, Nick::REGISTERED },
	{ MSG_STATS,   &IRC::m_stats,   0, 0, Nick::REGISTERED },
	{ MSG_CONNECT, &IRC::m_connect, 1, 0, Nick::REGISTERED },
	{ MSG_SQUIT,   &IRC::m_squit,   1, 0, Nick::REGISTERED },
	{ MSG_MAP,     &IRC::m_map,     0, 0, Nick::REGISTERED },
	{ MSG_ADMIN,   &IRC::m_admin,   0, 0, Nick::REGISTERED },
	{ MSG_JOIN,    &IRC::m_join,    1, 0, Nick::REGISTERED },
	{ MSG_PART,    &IRC::m_part,    1, 0, Nick::REGISTERED },
	{ MSG_NAMES,   &IRC::m_names,   1, 0, Nick::REGISTERED },
	{ MSG_TOPIC,   &IRC::m_topic,   1, 0, Nick::REGISTERED },
	{ MSG_LIST,    &IRC::m_list,    0, 0, Nick::REGISTERED },
	{ MSG_MODE,    &IRC::m_mode,    1, 0, Nick::REGISTERED },
	{ MSG_ISON,    &IRC::m_ison,    1, 0, Nick::REGISTERED },
	{ MSG_INVITE,  &IRC::m_invite,  2, 0, Nick::REGISTERED },
	{ MSG_KICK,    &IRC::m_kick,    2, 0, Nick::REGISTERED },
	{ MSG_KILL,    &IRC::m_kill,    1, 0, Nick::REGISTERED },
	{ MSG_SVSNICK, &IRC::m_svsnick, 2, 0, Nick::REGISTERED },
	{ MSG_AWAY,    &IRC::m_away,    0, 0, Nick::REGISTERED },
	{ MSG_MOTD,    &IRC::m_motd,    0, 0, Nick::REGISTERED },
	{ MSG_OPER,    &IRC::m_oper,    2, 0, Nick::REGISTERED },
	{ MSG_WALLOPS, &IRC::m_wallops, 1, 0, Nick::OPER },
	{ MSG_REHASH,  &IRC::m_rehash,  0, 0, Nick::OPER },
	{ MSG_DIE,     &IRC::m_die,     1, 0, Nick::OPER },
	{ NULL,        NULL,            0, 0, 0 },
};

IRC::IRC(ServerPoll* _poll, sock::SockWrapper* _sockw, string _hostname, unsigned _ping_freq)
	: Server("localhost.localdomain", MINBIF_VERSION),
	  poll(_poll),
	  sockw(_sockw),
	  read_cb(NULL),
	  ping_id(-1),
	  ping_freq(_ping_freq),
	  uptime(time(NULL)),
	  ping_cb(NULL),
	  user(NULL),
	  im(NULL),
	  im_auth(NULL)
{
	/* Get my own hostname (if not given in arguments) */
	if(_hostname.empty() || _hostname == " ")
		setName(sockw->GetServerHostname());
	else if(_hostname.find(" ") != string::npos)
	{
		/* An hostname can't contain any space. */
		b_log[W_ERR] << "'" << _hostname << "' is not a valid server hostname";
		throw sock::SockError("Wrong server hostname");
	}
	else
		setName(_hostname);

	/* create a callback on the sock. */
	read_cb = new CallBack<IRC>(this, &IRC::readIO);
	sockw->AttachCallback(PURPLE_INPUT_READ, read_cb);

	/* Create main objects and root joins command channel. */
	user = new User(sockw, this, "*", "", sockw->GetClientHostname());
	addNick(user);

	/* Ping callback */
	if(ping_freq > 0)
	{
		ping_cb = new CallBack<IRC>(this, &IRC::ping);
		ping_id = g_timeout_add((int)ping_freq * 1000, g_callback, ping_cb);
	}

	rehash(false);

	user->send(Message(MSG_NOTICE).setSender(this).setReceiver("AUTH").addArg("Minbif-IRCd initialized, please go on"));
}

IRC::~IRC()
{
	delete im;
	if (im_auth)
		delete im_auth;

	if(ping_id >= 0)
		g_source_remove(ping_id);
	delete ping_cb;
	if(sockw)
		delete sockw;
	delete read_cb;
	cleanUpNicks();
	cleanUpServers();
	cleanUpChannels();
	cleanUpDCC();
}

DCC* IRC::createDCCSend(const im::FileTransfert& ft, Nick* n)
{
	DCC* dcc = new DCCSend(ft, n, user);
	dccs.push_back(dcc);
	return dcc;
}

DCC* IRC::createDCCGet(Nick* from, string filename, uint32_t addr,
		       uint16_t port, ssize_t size, _CallBack* callback)
{
	DCC* dcc = new DCCGet(from, filename, addr, port, size, callback);
	dccs.push_back(dcc);
	return dcc;
}

void IRC::updateDCC(const im::FileTransfert& ft, bool destroy)
{
	for(vector<DCC*>::iterator it = dccs.begin(); it != dccs.end();)
	{
		/* Purge */
		if((*it)->isFinished())
		{
			delete *it;
			it = dccs.erase(it);
		}
		else
		{
			if((*it)->getFileTransfert() == ft)
				(*it)->updated(destroy);
			++it;
		}
	}
}

void IRC::cleanUpDCC()
{
	for(vector<DCC*>::iterator it = dccs.begin(); it != dccs.end(); ++it)
		delete *it;

	dccs.clear();
}

void IRC::addChannel(Channel* chan)
{
	if(channels.find(chan->getName()) != channels.end())
		b_log[W_DESYNCH] << "/!\\ Channel " << chan->getName() << " already exists!";
	channels[chan->getName()] = chan;
}

Channel* IRC::getChannel(string channame) const
{
	map<string, Channel*>::const_iterator it = channels.find(channame);
	if(it == channels.end())
		return 0;

	return it->second;
}

void IRC::removeChannel(string channame)
{
	map<string, Channel*>::iterator it = channels.find(channame);
	if(it != channels.end())
	{
		delete it->second;
		channels.erase(it);
	}
}

void IRC::cleanUpChannels()
{
	map<string, Channel*>::iterator it;
	for(it = channels.begin(); it != channels.end(); ++it)
		delete it->second;
	channels.clear();
}

void IRC::addNick(Nick* nick)
{
	if(users.find(nick->getNickname()) != users.end())
		b_log[W_DESYNCH] << "/!\\ User " << nick->getNickname() << " already exists!";
	users[nick->getNickname()] = nick;
	nick->getServer()->addNick(nick);
}

void IRC::renameNick(Nick* nick, string newnick)
{
	users.erase(nick->getNickname());
	nick->setNickname(newnick);
	addNick(nick);
}

Nick* IRC::getNick(string nickname, bool case_sensitive) const
{
	map<string, Nick*>::const_iterator it;
	if(!case_sensitive)
		nickname = strlower(nickname);
	for(it = users.begin(); it != users.end() && (case_sensitive ? it->first : strlower(it->first)) != nickname; ++it)
		;

	if(it == users.end())
		return 0;

	return it->second;
}

Buddy* IRC::getNick(const im::Buddy& buddy) const
{
	map<string, Nick*>::const_iterator it;
	Buddy* nb;
	for(it = users.begin();
	    it != users.end() && (!(nb = dynamic_cast<Buddy*>(it->second)) || nb->getBuddy() != buddy);
	    ++it)
		;

	if(it == users.end())
		return NULL;
	else
		return nb;
}

ConvNick* IRC::getNick(const im::Conversation& conv) const
{
	map<string, Nick*>::const_iterator it;
	ConvNick* n;
	for(it = users.begin();
	    it != users.end() && (!(n = dynamic_cast<ConvNick*>(it->second)) || n->getConversation() != conv);
	    ++it)
		;

	if(it == users.end())
		return NULL;
	else
		return n;
}

vector<Nick*> IRC::matchNick(string pattern) const
{
	map<string, Nick*>::const_iterator it;
	vector<Nick*> result;

	if (pattern.find('!') == string::npos)
	{
		if (pattern.find('@') != string::npos)
			pattern = "*!" + pattern;
		else if (pattern.find(':') != string::npos)
			pattern = "*!*@" + pattern;
		else
			pattern = pattern + "!*@*";
	}
	else if (pattern.find('@') == string::npos)
		pattern += "@*";

	for (it = users.begin(); it != users.end(); ++it)
	{
		string longname = it->second->getLongName();
		if (!fnmatch(pattern.c_str(), longname.c_str(), FNM_NOESCAPE|FNM_CASEFOLD))
			result.push_back(it->second);
	}

	return result;
}

void IRC::removeNick(string nickname)
{
	map<string, Nick*>::iterator it = users.find(nickname);
	if(it != users.end())
	{
		for(vector<DCC*>::iterator dcc = dccs.begin(); dcc != dccs.end();)
			if((*dcc)->isFinished())
			{
				delete *dcc;
				dcc = dccs.erase(dcc);
			}
			else
			{
				if((*dcc)->getPeer() == it->second)
					(*dcc)->setPeer(NULL);
				++dcc;
			}
		it->second->getServer()->removeNick(it->second);
		delete it->second;
		users.erase(it);
	}
}

void IRC::cleanUpNicks()
{
	map<string, Nick*>::iterator it;
	for(it = users.begin(); it != users.end(); ++it)
	{
		it->second->getServer()->removeNick(it->second);
		delete it->second;
	}
	users.clear();
}

void IRC::addServer(Server* server)
{
	servers[server->getName()] = server;
}

Server* IRC::getServer(string servername) const
{
	map<string, Server*>::const_iterator it = servers.find(servername);
	if(it == servers.end())
		return 0;

	return it->second;
}

void IRC::removeServer(string servername)
{
	map<string, Server*>::iterator it = servers.find(servername);
	if(it != servers.end())
	{
		/* Cleanup server's users */
		for(map<string, Nick*>::iterator nt = users.begin(); nt != users.end();)
			if(nt->second->getServer() == it->second)
			{
				delete nt->second;
				users.erase(nt);
				nt = users.begin();
			}
			else
				++nt;

		delete it->second;
		servers.erase(it);
	}
}

void IRC::cleanUpServers()
{
	map<string, Server*>::iterator it;
	for(it = servers.begin(); it != servers.end(); ++it)
		delete it->second;
	servers.clear();
}

void IRC::rehash(bool verbose)
{
	setMotd(conf.GetSection("path")->GetItem("motd")->String());
	if(verbose)
		b_log[W_INFO|W_SNO] << "Server configuration rehashed.";
}

void IRC::setMotd(const string& path)
{
	std::ifstream fp(path.c_str());
	if(!fp)
	{
		b_log[W_WARNING] << "Unable to read MOTD";
		return;
	}

	char buf[512];
	motd.clear();
	while(fp)
	{
		fp.getline(buf, 511);
		motd.push_back(buf);
	}
	fp.close();
}

void IRC::quit(string reason)
{
	user->send(Message(MSG_ERROR).addArg("Closing Link: " + reason));
	user->close();

	delete sockw;
	sockw = NULL;

	poll->kill(this);
}

void IRC::sendWelcome()
{
	if(user->hasFlag(Nick::REGISTERED) || user->getNickname() == "*" ||
	   user->getIdentname().empty())
		return;

	try
	{
		im_auth = im::Auth::validate(this, user->getNickname(), user->getPassword());
		if (!im_auth)
		{
			if (im::IM::exists(user->getNickname()))
			{
				quit("Incorrect credentials");
				return;
			}

			/* New User */
			string global_passwd = conf.GetSection("irc")->GetItem("password")->String();
			if(global_passwd != " " && user->getPassword() != global_passwd)
			{
				quit("This server is protected by a global private password.  Ask administrator.");
				return;
			}

			im_auth = im::Auth::generate(this, user->getNickname(), user->getPassword());
			if (!im_auth)
				quit("Creation of new account failed");
		}

		im = im_auth->getIM();

		user->setFlag(Nick::REGISTERED);
		poll->ipc_send(Message(MSG_USER).addArg(getUser()->getNickname()));

		// http://irchelp.org/irchelp/rfc/rfc2812.txt 5.1 -
		// "The server sends Replies 001 to 004 to a user upon successful registration."
		user->send(Message(RPL_WELCOME).setSender(this).setReceiver(user).addArg("Welcome to the Minbif IRC gateway, " + user->getNickname() + "!"));
		user->send(Message(RPL_YOURHOST).setSender(this).setReceiver(user).addArg("Your host is " + getServerName() + ", running " MINBIF_VERSION));
		user->send(Message(RPL_CREATED).setSender(this).setReceiver(user).addArg("This server was created " __DATE__ " " __TIME__));
		user->send(Message(RPL_MYINFO).setSender(this).setReceiver(user).addArg(getServerName() + " " +
		                                                                        MINBIF_VERSION + " " +
											Nick::UMODES + " " +
											Channel::CHMODES));
		user->send(Message(RPL_ISUPPORT).setSender(this).setReceiver(user).addArg("CMDS=MAP")
				                                                  /* TODO it doesn't compile because g++ is crappy.
										   * .addArg("NICKLEN=" + t2s(Nick::MAX_LENGTH)) */
										  .addArg("CHANTYPE=#&")
										  .addArg("PREFIX=(qohv)~@%+")
										  .addArg("STATUSMSG=~@%+")
										  .addArg("are supported by this server"));

		m_motd(Message());

		im->restore();

		if (im->isAway())
		{
			user->setAwayMessage("Away");
			user->send(Message(RPL_NOWAWAY).setSender(this)
					               .setReceiver(user)
						       .addArg("You have been marked as being away"));
		}
	}
	catch(im::IMError& e)
	{
		quit("Unable to initialize IM: " + e.Reason());
	}
}

bool IRC::ping(void*)
{
	if(user->getLastRead() + ping_freq > time(NULL))
		return true;

	if(!user->hasFlag(Nick::REGISTERED) || user->hasFlag(Nick::PING))
	{
		quit("Ping timeout");
		return false;
	}
	else
	{
		user->setFlag(Nick::PING);
		user->send(Message(MSG_PING).addArg(getServerName()));
		return true;
	}
}

void IRC::notice(Nick* nick, string msg)
{
	string tmp;
	while((tmp = stringtok(msg, "\n\r")).empty() == false)
		nick->send(Message(MSG_NOTICE).setSender(this)
					      .setReceiver(user)
					      .addArg(tmp));
}

void IRC::privmsg(Nick* nick, string msg)
{
	string tmp;
	while((tmp = stringtok(msg, "\n\r")).empty() == false)
		nick->send(Message(MSG_PRIVMSG).setSender(this)
					       .setReceiver(user)
					       .addArg(tmp));
}

bool IRC::readIO(void*)
{
	try
	{
		string sbuf, line;

		sbuf = sockw->Read();

		while((line = stringtok(sbuf, "\r\n")).empty() == false)
		{
			Message m = Message::parse(line);
			b_log[W_PARSE] << "<< " << line;
			size_t i;
			for(i = 0;
			    i < (sizeof commands / sizeof *commands) &&
			    strcmp(commands[i].cmd, m.getCommand().c_str());
			    ++i)
				;

			user->setLastReadNow();

			if(i >= (sizeof commands / sizeof *commands))
				user->send(Message(ERR_UNKNOWNCOMMAND).setSender(this)
								   .setReceiver(user)
								   .addArg(m.getCommand())
								   .addArg("Unknown command"));
			else if(m.countArgs() < commands[i].minargs)
				user->send(Message(ERR_NEEDMOREPARAMS).setSender(this)
								   .setReceiver(user)
								   .addArg(m.getCommand())
								   .addArg("Not enough parameters"));
			else if(commands[i].flags && !user->hasFlag(commands[i].flags))
			{
				if(!user->hasFlag(Nick::REGISTERED))
					user->send(Message(ERR_NOTREGISTERED).setSender(this)
									     .setReceiver(user)
									     .addArg("Register first"));
				else
					user->send(Message(ERR_NOPRIVILEGES).setSender(this)
									    .setReceiver(user)
									    .addArg("Permission Denied: Insufficient privileges"));
			}
			else
			{
				commands[i].count++;
				(this->*commands[i].func)(m);
			}
		}
	}
	catch (sock::SockError &e)
	{
		quit(e.Reason());
	}

	return true;
}

}; /* namespace irc */
