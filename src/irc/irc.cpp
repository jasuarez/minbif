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
#include <cassert>
#include <fstream>

#include "core/log.h"
#include "core/util.h"
#include "core/callback.h"
#include "core/version.h"
#include "core/config.h"
#include "im/im.h"
#include "server_poll/poll.h"
#include "irc/irc.h"
#include "irc/settings.h"
#include "irc/buddy.h"
#include "irc/dcc.h"
#include "irc/chat_buddy.h"
#include "irc/unknown_buddy.h"
#include "irc/message.h"
#include "irc/user.h"
#include "irc/channel.h"
#include "irc/status_channel.h"
#include "irc/conversation_channel.h"
#include "core/caca_image.h"

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
		throw sock::SockError::SockError("Wrong server hostname");
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

Nick* IRC::getNick(const im::Buddy& buddy) const
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
		return it->second;
}

Nick* IRC::getNick(const im::Conversation& conv) const
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
		return it->second;
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

		user->send(Message(RPL_WELCOME).setSender(this).setReceiver(user).addArg("Welcome to the Minbif IRC gateway, " + user->getNickname() + "!"));
		user->send(Message(RPL_YOURHOST).setSender(this).setReceiver(user).addArg("Your host is " + getServerName() + ", running " MINBIF_VERSION));
		user->send(Message(RPL_CREATED).setSender(this).setReceiver(user).addArg("This server was created " __DATE__ " " __TIME__));

		m_motd(Message());

		im->restore();
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

/** PING [args ...] */
void IRC::m_ping(Message message)
{
	message.setCommand(MSG_PONG);
	message.setSender(this);
	message.setReceiver(this);
	user->send(message);
}

/** PONG cookie */
void IRC::m_pong(Message message)
{
	user->delFlag(Nick::PING);
}

/** NICK nickname */
void IRC::m_nick(Message message)
{
	if(message.countArgs() < 1)
		user->send(Message(ERR_NONICKNAMEGIVEN).setSender(this)
						    .setReceiver(user)
						    .addArg("No nickname given"));
	else if(user->hasFlag(Nick::REGISTERED))
		user->send(Message(ERR_NICKTOOFAST).setSender(this)
						   .setReceiver(user)
						   .addArg("The hand of the deity is upon thee, thy nick may not change"));
	else if(!Nick::isValidNickname(message.getArg(0)))
		user->send(Message(ERR_ERRONEUSNICKNAME).setSender(this)
							.setReceiver(user)
							.addArg("This nick contains invalid characters"));
	else
	{
		renameNick(user, message.getArg(0));

		sendWelcome();
	}
}

/** USER identname * * :realname*/
void IRC::m_user(Message message)
{
	if(user->hasFlag(Nick::REGISTERED))
	{
		user->send(Message(ERR_ALREADYREGISTRED).setSender(this)
						     .setReceiver(user)
						     .addArg("Please register only once per session"));
		return;
	}

	user->setIdentname(message.getArg(0));
	user->setRealname(message.getArg(3));

	sendWelcome();
}

/** PASS passwd */
void IRC::m_pass(Message message)
{
	string password = message.getArg(0);
	if(user->hasFlag(Nick::REGISTERED))
		user->send(Message(ERR_ALREADYREGISTRED).setSender(this)
						     .setReceiver(user)
						     .addArg("Please register only once per session"));
	else if(password.size() < 8)
		quit("Password is too short (at least 8 characters)");
	else if(password.find(' ') != string::npos)
		quit("Password may not contain spaces");
	else
		user->setPassword(message.getArg(0));
}

/** QUIT [message] */
void IRC::m_quit(Message message)
{
	string reason = "Leaving...";
	if(message.countArgs() >= 1)
		reason = message.getArg(0);
	quit("Quit: " + reason);
}

/** VERSION */
void IRC::m_version(Message message)
{
	user->send(Message(RPL_VERSION).setSender(this)
				       .setReceiver(user)
				       .addArg(MINBIF_VERSION)
				       .addArg(getServerName())
				       .addArg(MINBIF_BUILD));
}

/** INFO */
void IRC::m_info(Message message)
{
	for(size_t i = 0; infotxt[i] != NULL; ++i)
		user->send(Message(RPL_INFO).setSender(this)
				            .setReceiver(user)
					    .addArg(infotxt[i]));

	user->send(Message(RPL_ENDOFINFO).setSender(this)
			                 .setReceiver(user)
					 .addArg("End of /INFO list."));
}

/** WHO */
void IRC::m_who(Message message)
{
	string arg;
	Channel* chan = NULL;
	if(message.countArgs() > 0)
		arg = message.getArg(0);

	if(arg.empty() || !Channel::isChanName(arg) || (chan = getChannel(arg)))
		for(std::map<string, Nick*>::iterator it = users.begin(); it != users.end(); ++it)
		{
			Nick* n = it->second;
			string channame = "*";
			if(arg.empty() || arg == "*" || arg == "0" || arg == n->getNickname() || n->getServer()->getServerName().find(arg) != string::npos)
			{
				vector<ChanUser*> chans = n->getChannels();
				if(!chans.empty())
					channame = chans.front()->getChannel()->getName();
			}
			else if(chan)
			{
				if(!n->isOn(chan))
					continue;
				channame = arg;
			}
			else
				continue;

			user->send(Message(RPL_WHOREPLY).setSender(this)
							.setReceiver(user)
							.addArg(channame)
							.addArg(n->getIdentname())
							.addArg(n->getHostname())
							.addArg(n->getServer()->getServerName())
							.addArg(n->getNickname())
							.addArg(n->isAway() ? "G" : "H")
							.addArg("0 " + n->getRealName()));
		}
	user->send(Message(RPL_ENDOFWHO).setSender(this)
					.setReceiver(user)
					.addArg(!arg.empty() ? arg : "**")
					.addArg("End of /WHO list"));
}

/** WHOIS nick */
void IRC::m_whois(Message message)
{
	Nick* n = getNick(message.getArg(0));
	if(!n)
	{
		user->send(Message(ERR_NOSUCHNICK).setSender(this)
						  .setReceiver(user)
						  .addArg(message.getArg(0))
						  .addArg("Nick does not exist"));
		return;
	}
	bool extended_whois = false;
	if(message.countArgs() > 1)
		extended_whois = true;

	user->send(Message(RPL_WHOISUSER).setSender(this)
					 .setReceiver(user)
					 .addArg(n->getNickname())
					 .addArg(n->getIdentname())
					 .addArg(n->getHostname())
					 .addArg("*")
					 .addArg(n->getRealName()));
	vector<ChanUser*> chanusers = n->getChannels();
	string chans;
	FOREACH(vector<ChanUser*>, chanusers, chanuser)
	{
		if(chans.empty() == false) chans += " ";
		chans += (*chanuser)->getChannel()->getName();
	}
	if(chans.empty() == false)
		user->send(Message(RPL_WHOISCHANNELS).setSender(this)
				                     .setReceiver(user)
						     .addArg(n->getNickname())
						     .addArg(chans));
	user->send(Message(RPL_WHOISSERVER).setSender(this)
					   .setReceiver(user)
					   .addArg(n->getNickname())
					   .addArg(n->getServer()->getServerName())
					   .addArg(n->getServer()->getServerInfo()));

	if(n->isAway())
		user->send(Message(RPL_AWAY).setSender(this)
					    .setReceiver(user)
					    .addArg(n->getNickname())
					    .addArg(n->getAwayMessage()));
	if(n->hasFlag(Nick::OPER))
		user->send(Message(RPL_WHOISOPERATOR).setSender(this)
				                     .setReceiver(user)
						     .addArg(n->getNickname())
						     .addArg("is an IRC Operator"));

	CacaImage icon = n->getIcon();
	try
	{
		string buf = icon.getIRCBuffer(0, extended_whois ? 15 : 10);
		string line;
		user->send(Message(RPL_WHOISACTUALLY).setSender(this)
					       .setReceiver(user)
					       .addArg(n->getNickname())
					       .addArg("Icon:"));
		while((line = stringtok(buf, "\r\n")).empty() == false)
		{
			user->send(Message(RPL_WHOISACTUALLY).setSender(this)
						       .setReceiver(user)
						       .addArg(n->getNickname())
						       .addArg(line));
		}
	}
	catch(CacaError &e)
	{
		user->send(Message(RPL_WHOISACTUALLY).setSender(this)
					       .setReceiver(user)
					       .addArg(n->getNickname())
					       .addArg("No icon"));
	}
	catch(CacaNotLoaded &e)
	{
		user->send(Message(RPL_WHOISACTUALLY).setSender(this)
					       .setReceiver(user)
					       .addArg(n->getNickname())
					       .addArg("libcaca and imlib2 are required to display icon"));
	}
	string url = conf.GetSection("irc")->GetItem("buddy_icons_url")->String();
	string icon_path = n->getIconPath();
	if(url != " " && !icon_path.empty())
	{
		icon_path = icon_path.substr(im->getUserPath().size());
		user->send(Message(RPL_WHOISACTUALLY).setSender(this)
						       .setReceiver(user)
						       .addArg(n->getNickname())
						       .addArg("Icon URL: " + url + im->getUsername() + icon_path));
	}

	/* Retrieve server info about this buddy only if this is an extended
	 * whois. In this case, do not send a ENDOFWHOIS because this
	 * is an asynchronous call.
	 */
	if(!extended_whois || !n->retrieveInfo())
		user->send(Message(RPL_ENDOFWHOIS).setSender(this)
						  .setReceiver(user)
						  .addArg(n->getNickname())
						  .addArg("End of /WHOIS list"));

}

/** WHOWAS nick
 *
 * As irsii tries a whowas when whois fails and waits for answer...
 */
void IRC::m_whowas(Message message)
{
	user->send(Message(ERR_WASNOSUCHNICK).setSender(this)
					     .setReceiver(user)
					     .addArg(message.getArg(0))
					     .addArg("Nick does not exist"));
	user->send(Message(RPL_ENDOFWHOWAS).setSender(this)
					   .setReceiver(user)
					   .addArg(message.getArg(0))
					   .addArg("End of WHOWAS"));
}

/** PRIVMSG target message */
void IRC::m_privmsg(Message message)
{
	Message relayed(message.getCommand());
	string targets = message.getArg(0), target;

	while ((target = stringtok(targets, ",")).empty() == false)
	{
		relayed.setSender(user);
		relayed.addArg(message.getArg(1));

		if(Channel::isChanName(target))
		{
			Channel* c = getChannel(target);
			if(!c)
			{
				user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
								     .setReceiver(user)
								     .addArg(target)
								     .addArg("No suck channel"));
				return;
			}
			relayed.setReceiver(c);
			c->broadcast(relayed, user);
		}
		else
		{
			Nick* n = getNick(target);
			if(!n)
			{
				user->send(Message(ERR_NOSUCHNICK).setSender(this)
								  .setReceiver(user)
								  .addArg(target)
								  .addArg("No suck nick"));
				return;
			}
			relayed.setReceiver(n);
			n->send(relayed);
			if(n->isAway())
				user->send(Message(RPL_AWAY).setSender(this)
					    .setReceiver(user)
					    .addArg(n->getNickname())
					    .addArg(n->getAwayMessage()));
		}
	}
}

/** CMD <target> <cmd> [args ...] */
void IRC::m_cmd(Message message)
{
	string target = message.getArg(0);
	int ret;
	string cmd;
	vector<string> args = message.getArgs();
	for(vector<string>::iterator it = args.begin()+1; it != args.end(); ++it)
	{
		if(!cmd.empty())
			cmd += ' ';
		cmd += *it;
	}

	if(Channel::isChanName(target))
	{
		Channel* c = getChannel(target);
		if(!c)
		{
			user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
							     .setReceiver(user)
							     .addArg(target)
							     .addArg("No suck channel"));
			return;
		}
		ret = c->sendCommand(cmd);
	}
	else
	{
		Nick* n = getNick(target);
		if(!n)
		{
			user->send(Message(ERR_NOSUCHNICK).setSender(this)
							  .setReceiver(user)
							  .addArg(target)
							  .addArg("No suck nick"));
			return;
		}
		ret = n->sendCommand(cmd);
	}
	switch (ret)
	{
		case PURPLE_CMD_STATUS_OK:
			break;
		case PURPLE_CMD_STATUS_NOT_FOUND:
			user->send(Message(ERR_UNKNOWNCOMMAND).setSender(this)
							   .setReceiver(user)
							   .addArg(message.getArg(1))
							   .addArg("Unknown command"));
			break;
		case PURPLE_CMD_STATUS_WRONG_ARGS:
			user->send(Message(ERR_NEEDMOREPARAMS).setSender(this)
				   .setReceiver(user)
				   .addArg(message.getArg(1))
				   .addArg("Not enough parameters"));
			break;
		case PURPLE_CMD_STATUS_FAILED:
			user->send(Message(ERR_NEEDMOREPARAMS).setSender(this)
				   .setReceiver(user)
				   .addArg(message.getArg(1))
				   .addArg("Command failed."));
			break;
		case PURPLE_CMD_STATUS_WRONG_TYPE:
			user->send(Message(ERR_NOPRIVILEGES).setSender(this)
							    .setReceiver(user)
							    .addArg("Permission Denied: This command doesn't work on this kind of target"));
			break;
		case PURPLE_CMD_STATUS_WRONG_PRPL:
			user->send(Message(ERR_NOPRIVILEGES).setSender(this)
							    .setReceiver(user)
							    .addArg("Permission Denied: That command doesn't work on this protocol."));
			break;
	}
}

/** STATS [p] */
void IRC::m_stats(Message message)
{
	string arg = "*";
	if(message.countArgs() > 0)
		arg = message.getArg(0);

	switch(arg[0])
	{
		case 'a':
			for(unsigned i = 0; i < (unsigned)PURPLE_STATUS_NUM_PRIMITIVES; ++i)
				notice(user, string(purple_primitive_get_id_from_type((PurpleStatusPrimitive)i)) +
					     ": " + purple_primitive_get_name_from_type((PurpleStatusPrimitive)i));
			break;
		case 'c':
		{
			string accid = message.countArgs() > 1 ? message.getArg(1) : "";
			im::Account account = im->getAccount(accid);
			if(!account.isValid() || !account.isConnected())
				user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
								     .setReceiver(user)
								     .addArg(accid)
								     .addArg("No such account"));
			else
			{
				map<string, string> m = account.getChatParameters();
				for(map<string, string>::iterator it = m.begin();
				    it != m.end();
				    ++it)
					notice(user, it->first + " = " + it->second);
			}
			break;
		}
		case 'm':
			for(size_t i = 0; i < sizeof commands / sizeof *commands; ++i)
				user->send(Message(RPL_STATSCOMMANDS).setSender(this)
						                     .setReceiver(user)
								     .addArg(commands[i].cmd)
								     .addArg(t2s(commands[i].count))
								     .addArg("0"));
			break;
		case 'o':
		{
			vector<ConfigSection*> opers = conf.GetSection("irc")->GetSectionClones("oper");
			for(vector<ConfigSection*>::iterator it = opers.begin(); it != opers.end(); ++it)
			{
				ConfigSection* oper = *it;

				user->send(Message(RPL_STATSOLINE).setSender(this)
						                  .setReceiver(user)
								  .addArg("O")
								  .addArg(oper->GetItem("email")->String())
								  .addArg("*")
								  .addArg(oper->GetItem("login")->String()));
			}
			break;
		}
		case 'p':
		{
			map<string, im::Protocol> m = im->getProtocolsList();
			for(map<string, im::Protocol>::iterator it = m.begin();
			    it != m.end(); ++it)
			{
				im::Protocol proto = it->second;
				notice(user, proto.getID() + ": " + proto.getName());
			}
			break;
		}
		case 'u':
		{
			unsigned now = time(NULL) - uptime;
			gchar *msg = g_strdup_printf("Server Up %d days, %d:%02d:%02d", now / 86400, (now / 3600) % 24, (now / 60) % 60, now % 60);
			user->send(Message(RPL_STATSUPTIME).setSender(this)
			                                   .setReceiver(user)
							   .addArg(msg));
			g_free(msg);
			break;
		}
		default:
			arg = "*";
			notice(user, "a (aways) - List all away messages availables");
			notice(user, "c (chat params) - List all chat parameters for a specific account");
			notice(user, "m (commands) - List all IRC commands");
			notice(user, "o (opers) - List all opers accounts");
			notice(user, "p (protocols) - List all protocols");
			notice(user, "u (uptime) - Display the server uptime");
			break;
	}
	user->send(Message(RPL_ENDOFSTATS).setSender(this)
					  .setReceiver(user)
					  .addArg(arg)
					  .addArg("End of /STATS report"));
}

/** CONNECT servername */
void IRC::m_connect(Message message)
{
	bool found = false;
	string target = message.getArg(0);

	map<string, im::Account> accounts = im->getAccountsList();
	for(map<string, im::Account>::iterator it = accounts.begin();
	    it != accounts.end(); ++it)
	{
		im::Account& account = it->second;
		if(target == "*" || account.getID() == target || account.getServername() == target)
		{
			found = true;
			account.connect();
			Channel* chan = getChannel(account.getStatusChannel());
			if(chan)
				user->join(chan, ChanUser::OP);

		}
	}

	if(!found && target != "*")
		notice(user, "Error: Account " + target + " is unknown");
}

/** SQUIT servername */
void IRC::m_squit(Message message)
{
	bool found = false;
	string target = message.getArg(0);

	map<string, im::Account> accounts = im->getAccountsList();
	for(map<string, im::Account>::iterator it = accounts.begin();
	    it != accounts.end(); ++it)
	{
		im::Account& account = it->second;
		if(target == "*" || account.getID() == target || account.getServername() == target)
		{
			found = true;
			account.disconnect();
		}
	}

	if(!found && target != "*")
		notice(user, "Error: Account " + target + " is unknown");
}

/** MAP */
void IRC::m_map(Message message)
{
	im::Account added_account;
	bool register_account = false;
	if(message.countArgs() > 0)
	{
		string arg = message.getArg(0);
		switch(arg[0])
		{
			case 'r':
			case 'R':
				register_account = true;
			case 'A':
			case 'a':
			{
				/* XXX Probably not needed. */
				message.rebuildWithQuotes();
				if(message.countArgs() < 2)
				{
					notice(user, "Usage: /MAP add PROTO USERNAME [OPTIONS]");
					notice(user, "To display available options: /MAP add PROTO");
					return;
				}
				string protoname = message.getArg(1);
				im::Protocol proto;
				try
				{
					 proto = im->getProtocol(protoname);
				}
				catch(im::ProtocolUnknown &e)
				{
					notice(user, "Error: Protocol " + protoname +
						     " is unknown. Try '/STATS p' to list protocols.");
					return;
				}

				im::Protocol::Options options = proto.getOptions();
				if(message.countArgs() < 3)
				{
					string s;
					FOREACH(im::Protocol::Options, options, it)
					{
						if(!s.empty()) s += " ";
						s += "[-";
						switch(it->second.getType())
						{
							case im::Protocol::Option::BOOL:
								s += "[!]" + it->second.getName();
								break;
							case im::Protocol::Option::ACCID:
							case im::Protocol::Option::STATUS_CHANNEL:
							case im::Protocol::Option::PASSWORD:
							case im::Protocol::Option::STR:
								s += it->second.getName() + " value";
								break;
							case im::Protocol::Option::INT:
								s += it->second.getName() + " int";
								break;
							case im::Protocol::Option::NONE:
								break;
						}
						s += "]";
					}
					notice(user, "Usage: /MAP add " + proto.getID() + " USERNAME " + s);
					return;
				}
				string username, password, channel;
				try {
					for(size_t i = 2; i < message.countArgs(); ++i)
					{
						string s = message.getArg(i);
						if(username.empty())
							username = s;
						else if(s[0] == '-')
						{
							size_t name_pos = 1;
							string value = "true";
							if(s[1] == '!')
							{
								value = "false";
								name_pos++;
							}

							im::Protocol::Options::iterator it = options.find(s.substr(name_pos));
							if (it == options.end())
							{
								notice(user, "Error: Option '" + s + "' does not exist");
								return;
							}
							if(it->second.getType() == im::Protocol::Option::BOOL)
							{
								/* No input value needed, already got above */
							}
							else if(i+1 < message.countArgs())
								value = message.getArg(++i);
							else
							{
								notice(user, "Error: Option '" + s + "' needs a value");
								return;
							}
							it->second.setValue(value);
						}
						else if (password.empty())
							options["password"].setValue(s);
						else if (channel.empty())
							options["status_channel"].setValue(s);
					}

					added_account = im->addAccount(proto, username, options, register_account);
				} catch (im::Protocol::OptionError& e) {
					notice(user, "Error: " + e.Reason());
					return;
				}

				break;
			}
			case 'E':
			case 'e':
			{
				if(message.countArgs() < 2)
				{
					notice(user, "Usage: /MAP edit ACCOUNT [KEY [VALUE]]");
					break;
				}
				im::Account account = im->getAccount(message.getArg(1));
				if(!account.isValid())
				{
					notice(user, "Error: Account " + message.getArg(1) + " is unknown");
					return;
				}
				im::Protocol::Options options = account.getOptions();
				if(message.countArgs() < 3)
				{
					notice(user, "-- Parameters of account " + account.getServername() + " --");
					FOREACH(im::Protocol::Options, options, it)
					{
						im::Protocol::Option& option = it->second;
						string value;
						switch(option.getType())
						{
							case im::Protocol::Option::PASSWORD:
								value = "*******";
								break;
							default:
								value = option.getValue();
								break;
						}
						notice(user, option.getName() + " = " + value);
					}
					return;
				}
				im::Protocol::Options::iterator option;
				for(option = options.begin();
				    option != options.end() && option->second.getName() != message.getArg(2);
				    ++option)
					;

				if(option == options.end())
				{
					notice(user, "Key " + message.getArg(2) + " does not exist.");
					return;
				}

				if(message.countArgs() < 4)
					notice(user, option->second.getName() + " = " + option->second.getValue());
				else
				{
					string value;
					for(unsigned i = 3; i < message.countArgs(); ++i)
					{
						if(!value.empty()) value += " ";
						value += message.getArg(i);
					}

					if(option->second.getType() == im::Protocol::Option::BOOL && value != "true" && value != "false")
					{
						notice(user, "Error: Option '" + option->second.getName() + "' is a boolean ('true' or 'false')");
						return;
					}
					/* TODO check if value is an integer if option is an integer */
					option->second.setValue(value);
					if(option->second.getType() == im::Protocol::Option::INT)
						notice(user, option->second.getName() + " = " + t2s(option->second.getValueInt()));
					else
						notice(user, option->second.getName() + " = " + option->second.getValue());
					account.setOptions(options);
				}
				return;
			}
			case 'D':
			case 'd':
			{
				if(message.countArgs() != 2)
				{
					notice(user, "Usage: /MAP delete ACCOUNT");
					return;
				}
				im::Account account = im->getAccount(message.getArg(1));
				if (!account.isValid())
				{
					notice(user, "Error: Account " + message.getArg(1) + " is unknown");
					return;
				}
				notice (user, "Removing account "+account.getUsername());
				im->delAccount(account);
				break;
			}
			case 'c':
			case 'C':
			{
				if(message.countArgs() < 2)
				{
					notice(user, "Usage: /MAP cmd ACCOUNT [command]");
					return;
				}
				im::Account account = im->getAccount(message.getArg(1));
				if(!account.isValid())
				{
					notice(user, "Error: Account " + message.getArg(1) + " is unknown");
					return;
				}
				if(!account.isConnected())
				{
					notice(user, "Error: Account " + message.getArg(1) + " is not connected");
					return;
				}
				if(message.countArgs() < 3)
				{
					vector<string> cmds = account.getCommandsList();
					notice(user, "Commands available for " + message.getArg(1) + ":");
					for(vector<string>::iterator it = cmds.begin(); it != cmds.end(); ++it)
						notice(user, "  " + *it);
				}
				else if (!account.callCommand(message.getArg(2)))
					notice(user, "Command " + message.getArg(2) + " is unknown");
				return;
			}
			case 'H':
			case 'h':
				notice(user,"add PROTO USERNAME PASSWD [CHANNEL] [options]   add an account");
				notice(user,"reg PROTO USERNAME PASSWD [CHANNEL] [options]   add an account and register it on server");
				notice(user,"edit ACCOUNT [KEY [VALUE]]                      edit an account");
				notice(user,"cmd ACCOUNT [COMMAND]                           run a command on an account");
				notice(user,"delete ACCOUNT                                  remove ACCOUNT from your accounts");
				notice(user,"help                                            display this message");
			default:
				notice(user,"Usage: /MAP add|register|edit|command|delete|help [...]");
				break;
		}
	}

	user->send(Message(RPL_MAP).setSender(this)
				   .setReceiver(user)
				   .addArg(this->getServerName()));

	map<string, im::Account> accounts = im->getAccountsList();
	for(map<string, im::Account>::iterator it = accounts.begin();
	    it != accounts.end(); ++it)
	{
		map<string, im::Account>::iterator next = it;
		Server* server = getServer(it->second.getServername());
		string name;

		if(++next == accounts.end())
			name = "`";
		else
			name = "|";

		if(it->second == added_account)
			name += "-+";
		else if(it->second.isConnecting())
			name += "-*";
		else if(it->second.isConnected())
			name += "-";
		else
			name += " ";

		name += it->second.getServername();

		if(it->second.isConnected())
			name += "  [" + t2s(server->countOnlineNicks()) + "/" + t2s(server->countNicks()) + "]";

		user->send(Message(RPL_MAP).setSender(this)
					   .setReceiver(user)
					   .addArg(name));
	}
	user->send(Message(RPL_MAPEND).setSender(this)
				      .setReceiver(user)
				      .addArg("End of /MAP"));

}

/** ADMIN [key value] */
void IRC::m_admin(Message message)
{
	assert(im != NULL);

	/* XXX It should does not work with several instances of IRC */
	static struct
	{
		const char* key;
		bool display;
		SettingBase* setting;
	} settings[] = {
		{ "password",                  true,  new SettingPassword(this, im) },
		{ "typing_notice",             true,  new SettingTypingNotice(this, im) },
		{ "accept_nobuddies_messages", true,  new SettingAcceptNoBuddiesMessages(this, im) },
		{ "voiced_buddies",            true,  new SettingVoicedBuddies(this, im) },
		{ "away_idle",                 true,  new SettingAwayIdle(this, im) },
		{ "log_level",                 true,  new SettingLogLevel(this, im) },
		{ "proxy",                     true,  new SettingProxy(this, im) },
		{ "proxy_host",                true,  new SettingProxyHost(this, im) },
		{ "proxy_port",                true,  new SettingProxyPort(this, im) },
		{ "proxy_user",                true,  new SettingProxyUsername(this, im) },
		{ "proxy_pass",                true,  new SettingProxyPassword(this, im) },
		{ "minbif",                    false, new SettingMinbif(this, im) },
	};

	if(message.countArgs() == 0)
	{
		for(unsigned i = 0; i < (sizeof settings / sizeof *settings); ++i)
			if(settings[i].display)
				user->send(Message(RPL_ADMINME).setSender(this)
							       .setReceiver(user)
							       .addArg(string("- ") + settings[i].key + " = " + settings[i].setting->getValue()));
		return;
	}

	unsigned i;
	for(i = 0; i < (sizeof settings / sizeof *settings) && message.getArg(0) != settings[i].key; ++i)
		;

	if(i >= (sizeof settings / sizeof *settings))
	{
		notice(user, "Key " + message.getArg(0) + " does not exist.");
		return;
	}

	if(message.countArgs() == 1)
	{
		user->send(Message(RPL_ADMINME).setSender(this)
					       .setReceiver(user)
					       .addArg(string("- ") + settings[i].key + " = " + settings[i].setting->getValue()));
		return;
	}

	vector<string> args = message.getArgs();
	string value;
	for(vector<string>::iterator it = args.begin() + 1; it != args.end(); ++it)
	{
		if(!value.empty())
			value += " ";
		value += *it;
	}

	settings[i].setting->setValue(value);

	user->send(Message(RPL_ADMINME).setSender(this)
				       .setReceiver(user)
				       .addArg(string("- ") + settings[i].key + " = " + settings[i].setting->getValue()));
}

/** JOIN channame */
void IRC::m_join(Message message)
{
	string names = message.getArg(0);
	string channame;
	string parameters = message.countArgs() > 1 ? message.getArg(1) : "";
	while((channame = stringtok(names, ",")).empty() == false)
	{
		if(!Channel::isChanName(channame))
		{
			user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
							     .setReceiver(user)
							     .addArg(channame)
							     .addArg("No such channel"));
			continue;
		}

		switch(channame[0])
		{
			case '&':
			{
				Channel* chan = getChannel(channame);
				if(!chan)
				{
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
					continue;
				}
				user->join(chan, ChanUser::OP);
				break;
			}
			case '#':
			{
				Channel* chan = getChannel(channame);

				/* Channel already exists, I'm really probably in. */
				if(chan)
					continue;

				string accid = channame.substr(1);
				string convname = stringtok(accid, ":");
				string param = stringtok(parameters, ",");
				if(accid.empty() || convname.empty())
				{
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
					continue;
				}
				im::Account account = im->getAccount(accid);
				if(!account.isValid() || account.isConnected() == false)
				{
					if(account.isValid() && account.isConnecting())
						account.enqueueChannelJoin(convname);
					else
						user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
										     .setReceiver(user)
										     .addArg(channame)
										     .addArg("No such channel"));
					continue;
				}

				/* purple_url_decode() returns a static buffer, no free needed. */
				if(!account.joinChat(purple_url_decode(convname.c_str()), param))
				{
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
				}
				else
					g_timeout_add(1000, g_callback_delete, new CallBack<IRC>(this, &IRC::check_channel_join, g_strdup(channame.c_str())));

#if 0
				chan = new ConversationChannel(this, conv);

				user->send(Message(ERR_CHANFORWARDING).setSender(this)
						                      .setReceiver(user)
								      .addArg(channame)
								      .addArg(chan->getName())
								      .addArg("Forwarding to another channel"));
				user->join(chan);
#endif
				break;
			}
			default:
				user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
								     .setReceiver(user)
								     .addArg(channame)
								     .addArg("No such channel"));

				break;
		}
	}
}

bool IRC::check_channel_join(void* data)
{
	char* name = static_cast<char*>(data);

	if(!getChannel(name))
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
					             .setReceiver(user)
					             .addArg(name)
					             .addArg("No such channel"));

	g_free(data);
	return false;
}

/** PART chan [:message] */
void IRC::m_part(Message message)
{
	string channame = message.getArg(0);
	string reason = "";
	if(message.countArgs() > 1)
		reason = message.getArg(1);

	Channel* chan = getChannel(channame);
	if(!chan)
	{
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
						     .setReceiver(user)
						     .addArg(channame)
						     .addArg("No such channel"));
		return;
	}
	user->part(chan, reason);
}

/** LIST */
void IRC::m_list(Message message)
{
	if(message.countArgs() == 0)
	{
		user->send(Message(RPL_LISTSTART).setSender(this)
						 .setReceiver(user)
						 .addArg("Channel")
						 .addArg("Users  Name"));
		for(map<string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
			user->send(Message(RPL_LIST).setSender(this)
						    .setReceiver(user)
						    .addArg(it->second->getName())
						    .addArg(t2s(it->second->countUsers())));

		user->send(Message(RPL_LISTEND).setSender(this)
					       .setReceiver(user)
					       .addArg("End of /LIST"));
	}
	else
	{
		im::Account account = im->getAccount(message.getArg(0));
		if(!account.isValid() || !account.isConnected())
			user->send(Message(RPL_LISTEND).setSender(this)
						       .setReceiver(user)
						       .addArg("End of /LIST"));
		else
			account.displayRoomList();
	}
}

/** MODE target [modes ..] */
void IRC::m_mode(Message message)
{
	Message relayed(message.getCommand());
	string target = message.getArg(0);

	relayed.setSender(user);
	for(size_t i = 1; i < message.countArgs(); ++i)
		relayed.addArg(message.getArg(i));

	if(Channel::isChanName(target))
	{
		Channel* c = getChannel(target);
		if(!c)
		{
			user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
							     .setReceiver(user)
							     .addArg(target)
							     .addArg("No suck channel"));
			return;
		}
		relayed.setReceiver(c);
		c->m_mode(user, relayed);
	}
	else
	{
		Nick* n = getNick(target);
		if(!n)
		{
			user->send(Message(ERR_NOSUCHNICK).setSender(this)
							  .setReceiver(user)
							  .addArg(target)
							  .addArg("No suck nick"));
			return;
		}
		relayed.setReceiver(n);
		n->m_mode(user, relayed);
	}

}

/** ISON :[nick list] */
void IRC::m_ison(Message message)
{
	string buf = message.getArg(0);
	string nick;
	string list;
	while((nick = stringtok(buf, " ")).empty() == false)
	{
		Nick* n;
		if((n = getNick(nick)) && n->isOnline())
		{
			if(!list.empty())
				list += " ";
			list += n->getNickname();
		}
	}
	user->send(Message(RPL_ISON).setSender(this)
				    .setReceiver(user)
				    .addArg(list));
}

/** NAMES chan */
void IRC::m_names(Message message)
{
	Channel* chan = getChannel(message.getArg(0));
	if(!chan)
	{
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(0))
						     .addArg("No such channel"));
		return;
	}

	chan->sendNames(user);
}

/** TOPIC chan [message] */
void IRC::m_topic(Message message)
{
	Channel* chan = getChannel(message.getArg(0));
	if(!chan)
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(0))
						     .addArg("No such channel"));
	else if(message.countArgs() < 2)
	{
		string topic = chan->getTopic();
		if(topic.empty())
			user->send(Message(RPL_NOTOPIC).setSender(this)
					             .setReceiver(user)
						     .addArg(chan->getName())
						     .addArg("No topic set."));
		else
			user->send(Message(RPL_TOPIC).setSender(this)
						     .setReceiver(user)
						     .addArg(chan->getName())
						     .addArg(topic));
	}
	else if(!chan->setTopic(user, message.getArg(1)))
		user->send(Message(ERR_CHANOPRIVSNEEDED).setSender(this)
							.setReceiver(user)
							.addArg(chan->getName())
							.addArg("You have no rights to change this channel topic!"));

}

/** INVITE nick chan */
void IRC::m_invite(Message message)
{
	Channel* chan = getChannel(message.getArg(1));
	if(!chan)
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(1))
						     .addArg("No such channel"));
	else if(chan->invite(user, message.getArg(0), ""))
		user->send(Message(RPL_INVITING).setSender(this)
						.setReceiver(user)
						.addArg(message.getArg(0))
						.addArg(chan->getName()));
}

/** KICK chan nick [:reason] */
void IRC::m_kick(Message message)
{
	Channel* chan = getChannel(message.getArg(0));
	if(!chan)
	{
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(0))
						     .addArg("No such channel"));
		return;
	}

	ChanUser* user_chanuser = user->getChanUser(chan);
	if(!user_chanuser)
	{
		user->send(Message(ERR_NOTONCHANNEL).setSender(this)
				                    .setReceiver(user)
						    .addArg(chan->getName())
						    .addArg("You're not on that channel"));
		return;
	}

	ChanUser* chanuser = chan->getChanUser(message.getArg(1));
	if(!chanuser)
	{
		user->send(Message(ERR_NOSUCHNICK).setSender(this)
				                  .setReceiver(user)
						  .addArg(message.getArg(1))
						  .addArg("No such nick"));
		return;
	}

	chan->kick(user_chanuser, chanuser, message.countArgs() > 2 ? message.getArg(2) : "");
}

/** KILL nick [:reason] */
void IRC::m_kill(Message message)
{
	Nick* n = getNick(message.getArg(0));
	if(!n)
	{
		user->send(Message(ERR_NOSUCHNICK).setSender(this)
				                  .setReceiver(user)
						  .addArg(message.getArg(0))
						  .addArg("No such nick"));
		return;
	}
	Buddy* buddy = dynamic_cast<Buddy*>(n);
	if(!buddy)
	{
		user->send(Message(ERR_NOPRIVILEGES).setSender(this)
						    .setReceiver(user)
						    .addArg("Permission denied: you can only kill a buddy"));
		return;
	}

	RemoteServer* rt = dynamic_cast<RemoteServer*>(buddy->getServer());
	if(!rt)
	{
		notice(user, buddy->getName() + " is not on a remote server");
		return;
	}
	string reason = "Removed from buddy list";
	if(message.countArgs() > 1 && message.getArg(1).empty() == false)
		reason += ": " + message.getArg(1);

	notice(user, "Received KILL message for " + buddy->getNickname() + ": " + reason);
	buddy->quit("Killed by " + user->getNickname() + " (" + reason + ")");
	rt->getAccount().removeBuddy(buddy->getBuddy());
}

/** SVSNICK nick new_nick */
void IRC::m_svsnick(Message message)
{
	Nick* n = getNick(message.getArg(0));
	if(!n)
	{
		user->send(Message(ERR_NOSUCHNICK).setSender(this)
				                  .setReceiver(user)
						  .addArg(message.getArg(0))
						  .addArg("No such nick"));
		return;
	}
	Buddy* buddy = dynamic_cast<Buddy*>(n);
	if(!buddy)
	{
		user->send(Message(ERR_NOPRIVILEGES).setSender(this)
						    .setReceiver(user)
						    .addArg("Permission denied: you can only change buddy nickname"));
		return;
	}

	if(!Nick::isValidNickname(message.getArg(1)))
	{
		user->send(Message(ERR_ERRONEUSNICKNAME).setSender(this)
							.setReceiver(user)
							.addArg("This nick contains invalid characters"));
		return;
	}

	if(getNick(message.getArg(1), true))
	{
		user->send(Message(ERR_NICKNAMEINUSE).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(1))
						     .addArg("Nickname is already in use"));
		return;
	}

	user->send(Message(MSG_NICK).setSender(buddy)
				    .addArg(message.getArg(1)));

	renameNick(buddy, message.getArg(1));
	buddy->getBuddy().setAlias(message.getArg(1));
}

/** AWAY [message] */
void IRC::m_away(Message message)
{
	string away;
	if(message.countArgs())
		away = message.getArg(0);

	if(im->setStatus(away))
	{
		user->setAwayMessage(away);
		if(away.empty())
			user->send(Message(RPL_UNAWAY).setSender(this)
					              .setReceiver(user)
						      .addArg("You are no longer marked as being away"));
		else
			user->send(Message(RPL_NOWAWAY).setSender(this)
					               .setReceiver(user)
						       .addArg("You have been marked as being away"));
	}
}

/* MOTD */
void IRC::m_motd(Message message)
{
	user->send(Message(RPL_MOTDSTART).setSender(this).setReceiver(user).addArg("- " + getServerName() + " Message Of The Day -"));
	for(vector<string>::iterator s = motd.begin(); s != motd.end(); ++s)
		user->send(Message(RPL_MOTD).setSender(this).setReceiver(user).addArg("- " + *s));

	user->send(Message(RPL_ENDOFMOTD).setSender(this).setReceiver(user).addArg("End of /MOTD command."));
}

/* OPER login password */
void IRC::m_oper(Message message)
{
	if(user->hasFlag(Nick::OPER))
	{
		user->send(Message(RPL_YOUREOPER).setSender(this)
						 .setReceiver(user)
						 .addArg("You are already an IRC Operator"));
		return;
	}

	vector<ConfigSection*> opers = conf.GetSection("irc")->GetSectionClones("oper");
	for(vector<ConfigSection*>::iterator it = opers.begin(); it != opers.end(); ++it)
	{
		ConfigSection* oper = *it;

		if(oper->GetItem("login")->String() == message.getArg(0) &&
		   oper->GetItem("password")->String() == message.getArg(1))
		{
			user->setFlag(Nick::OPER);
			user->send(Message(MSG_MODE).setSender(user)
					            .setReceiver(user)
						    .addArg("+o"));
			user->send(Message(RPL_YOUREOPER).setSender(this)
					                 .setReceiver(user)
							 .addArg("You are now an IRC Operator"));
			poll->ipc_send(Message(MSG_OPER).addArg(user->getNickname()));
			return;
		}
	}

	user->send(Message(ERR_PASSWDMISMATCH).setSender(this)
			                      .setReceiver(user)
					      .addArg("Password incorrect"));
}

/* WALLOPS :message */
void IRC::m_wallops(Message message)
{
	if(!poll->ipc_send(Message(MSG_WALLOPS).addArg(getUser()->getNickname())
			                       .addArg(message.getArg(0))))
	{
		b_log[W_ERR] << "You're alone!";
	}
}

/* REHASH */
void IRC::m_rehash(Message message)
{
	getUser()->send(Message(RPL_REHASHING).setSender(this)
			                      .setReceiver(user)
					      .addArg("Rehashing"));
	poll->rehash();
}

/* DIE message */
void IRC::m_die(Message message)
{
	if(!poll->ipc_send(Message(MSG_DIE).addArg(getUser()->getNickname())
				           .addArg(message.getArg(0))))
	{
		b_log[W_INFO|W_SNO] << "This instance of MinBif is dying... Reason: " << message.getArg(0);
		quit("Shutdown requested: " + message.getArg(0));
	}
}

}; /* namespace irc */
