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

#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <cstring>
#include <algorithm>

#include "../log.h"
#include "../util.h"
#include "../callback.h"
#include "../version.h"
#include "../sock.h"
#include "im/im.h"
#include "server_poll/poll.h"
#include "irc.h"
#include "buddy.h"
#include "message.h"
#include "user.h"
#include "channel.h"
#include "status_channel.h"
#include "conversation_channel.h"
#include "caca_image.h"

namespace irc {

IRC::command_t IRC::commands[] = {
	{ MSG_NICK,    &IRC::m_nick,    0, 0, 0 },
	{ MSG_USER,    &IRC::m_user,    4, 0, 0 },
	{ MSG_PASS,    &IRC::m_pass,    1, 0, 0 },
	{ MSG_QUIT,    &IRC::m_quit,    0, 0, 0 },
	{ MSG_PRIVMSG, &IRC::m_privmsg, 2, 0, Nick::REGISTERED },
	{ MSG_PING,    &IRC::m_ping,    0, 0, Nick::REGISTERED },
	{ MSG_PONG,    &IRC::m_pong,    1, 0, Nick::REGISTERED },
	{ MSG_VERSION, &IRC::m_version, 0, 0, Nick::REGISTERED },
	{ MSG_WHO,     &IRC::m_who,     0, 0, Nick::REGISTERED },
	{ MSG_WHOIS,   &IRC::m_whois,   1, 0, Nick::REGISTERED },
	{ MSG_WHOWAS,  &IRC::m_whowas,  1, 0, Nick::REGISTERED },
	{ MSG_STATS,   &IRC::m_stats,   0, 0, Nick::REGISTERED },
	{ MSG_CONNECT, &IRC::m_connect, 1, 0, Nick::REGISTERED },
	{ MSG_SQUIT,   &IRC::m_squit,   1, 0, Nick::REGISTERED },
	{ MSG_MAP,     &IRC::m_map,     0, 0, Nick::REGISTERED },
	{ MSG_JOIN,    &IRC::m_join,    1, 0, Nick::REGISTERED },
	{ MSG_PART,    &IRC::m_part,    1, 0, Nick::REGISTERED },
	{ MSG_LIST,    &IRC::m_list,    0, 0, Nick::REGISTERED },
	{ MSG_MODE,    &IRC::m_mode,    1, 0, Nick::REGISTERED },
	{ MSG_ISON,    &IRC::m_ison,    1, 0, Nick::REGISTERED },
	{ MSG_INVITE,  &IRC::m_invite,  2, 0, Nick::REGISTERED },
	{ MSG_KICK,    &IRC::m_kick,    2, 0, Nick::REGISTERED },
	{ MSG_KILL,    &IRC::m_kill,    1, 0, Nick::REGISTERED },
	{ MSG_SVSNICK, &IRC::m_svsnick, 2, 0, Nick::REGISTERED },
};

IRC::IRC(ServerPoll* _poll, int _fd, string _hostname, unsigned _ping_freq)
	: Server("localhost.localdomain", MINBIF_VERSION),
	  poll(_poll),
	  fd(_fd),
	  read_id(0),
	  read_cb(NULL),
	  ping_id(0),
	  ping_freq(_ping_freq),
	  ping_cb(NULL),
	  user(NULL),
	  im(NULL)
{
	struct sockaddr_storage sock;
	socklen_t socklen = sizeof(sock);

	fcntl(0, F_SETFL, O_NONBLOCK);

	/* Get the user's hostname. */
	string userhost = "localhost.localdomain";
	if(getpeername(fd, (struct sockaddr*) &sock, &socklen) == 0)
	{
		char buf[NI_MAXHOST+1];

		if(getnameinfo((struct sockaddr *)&sock, socklen, buf, NI_MAXHOST, NULL, 0, 0) == 0)
			userhost = buf;
	}

	/* Get my own hostname (if not given in arguments) */
	if(_hostname.empty() || _hostname == " ")
	{
		if(getsockname(fd, (struct sockaddr*) &sock, &socklen) == 0)
		{
			char buf[NI_MAXHOST+1];

			if(getnameinfo((struct sockaddr *) &sock, socklen, buf, NI_MAXHOST, NULL, 0, 0 ) == 0)
				setName(buf);
		}
	}
	else if(_hostname.find(" ") != string::npos)
	{
		/* An hostname can't contain any space. */
		b_log[W_ERR] << "'" << _hostname << "' is not a valid server hostname";
		throw AuthError();
	}
	else
		setName(_hostname);

	/* create a callback on the sock. */
	read_cb = new CallBack<IRC>(this, &IRC::readIO);
	read_id = glib_input_add(fd, (PurpleInputCondition)PURPLE_INPUT_READ, g_callback_input, read_cb);

	/* Create main objects and root joins command channel. */
	user = new User(fd, this, "*", "", userhost);
	addNick(user);

	/* Ping callback */
	if(ping_freq > 0)
	{
		ping_cb = new CallBack<IRC>(this, &IRC::ping);
		ping_id = g_timeout_add_seconds((int)ping_freq, g_callback, ping_cb);
	}

	user->send(Message(MSG_NOTICE).setSender(this).setReceiver("AUTH").addArg("Minbif-IRCd initialized, please go on"));
}

IRC::~IRC()
{
	delete im;

	if(read_id > 0)
		g_source_remove(read_id);
	if(ping_id > 0)
		g_source_remove(ping_id);
	delete read_cb;
	delete ping_cb;
	if(fd >= 0)
		close(fd);
	cleanUpNicks();
	cleanUpServers();
	cleanUpChannels();
}

void IRC::addChannel(Channel* chan)
{
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
	users[nick->getNickname()] = nick;
}

Nick* IRC::getNick(string nickname) const
{
	map<string, Nick*>::const_iterator it;
	nickname = strlower(nickname);
	for(it = users.begin(); it != users.end() && strlower(it->first) != nickname; ++it)
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

void IRC::removeNick(string nickname)
{
	map<string, Nick*>::iterator it = users.find(nickname);
	if(it != users.end())
	{
		delete it->second;
		users.erase(it);
	}
}

void IRC::cleanUpNicks()
{
	map<string, Nick*>::iterator it;
	for(it = users.begin(); it != users.end(); ++it)
		delete it->second;
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


void IRC::quit(string reason)
{
	user->send(Message(MSG_ERROR).addArg("Closing Link: " + reason));

	if(read_id > 0)
		g_source_remove(read_id);
	read_id = 0;

	user->close();
	close(fd);
	fd = -1;

	poll->kill(this);
}

void IRC::sendWelcome()
{
	if(user->hasFlag(Nick::REGISTERED) || user->getNickname() == "*" ||
	   user->getIdentname().empty())
		return;

	if(user->getPassword().empty())
	{
		quit("Please set a password");
		return;
	}

	try
	{
		im = new im::IM(this, user->getNickname());

		if(im->getPassword().empty())
		{
			/* New user. */
			im->setPassword(user->getPassword());
		}
		else if(im->getPassword() != user->getPassword())
		{
			quit("Incorrect password");
			return;
		}

		user->setFlag(Nick::REGISTERED);

		user->send(Message(RPL_WELCOME).setSender(this).setReceiver(user).addArg("Welcome to the Minbif gateway, " + user->getNickname() + "!"));
		user->send(Message(RPL_YOURHOST).setSender(this).setReceiver(user).addArg("Host " + getServerName() + " is running Minbif"));

		user->send(Message(RPL_MOTDSTART).setSender(this).setReceiver(user).addArg("- " + getServerName() + " Message Of The Day -"));
		if (im->getAccountsList().empty())
			user->send(Message(RPL_MOTD).setSender(this).setReceiver(user).addArg("Advice: you should start by adding an account using the /MAP command."));

		user->send(Message(RPL_ENDOFMOTD).setSender(this).setReceiver(user).addArg("End of /MOTD command."));
	}
	catch(im::IMError& e)
	{
		quit("Unable to initialize IM");
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
	nick->send(Message(MSG_NOTICE).setSender(this)
				      .setReceiver(user)
				      .addArg(msg));
}

void IRC::privmsg(Nick* nick, string msg)
{
	nick->send(Message(MSG_PRIVMSG).setSender(this)
				       .setReceiver(user)
				       .addArg(msg));
}

bool IRC::readIO(void*)
{
	static char buf[1024];
	string sbuf, line;
	ssize_t r;

	if((r = read( 0, buf, sizeof buf - 1 )) <= 0)
	{
		if(r == 0)
			this->quit("Connection reset by peer...");
		else if(!sockerr_again())
			this->quit(string("Read error: ") + strerror(errno));
		else
			return true; // continue...
		return false;
	}
	buf[r] = 0;
	sbuf = buf;


	while((line = stringtok(sbuf, "\r\n")).empty() == false)
	{
		Message m = Message::parse(line);
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
			user->send(Message(ERR_NOTREGISTERED).setSender(this)
							     .setReceiver(user)
							     .addArg("Register first"));
		else
		{
			commands[i].count++;
			(this->*commands[i].func)(m);
		}
	}

	return true;
}

/** PING [args ...] */
void IRC::m_ping(Message message)
{
	message.setCommand(MSG_PONG);
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
		users.erase(user->getNickname());
		user->setNickname(message.getArg(0));
		users[message.getArg(0)] = user;

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
				       .addArg(getServerName()));
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
			if(arg.empty() || arg == "*" || arg == "0" || arg == n->getNickname())
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
							.addArg("0 " + n->getRealname()));
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

	user->send(Message(RPL_WHOISUSER).setSender(this)
					 .setReceiver(user)
					 .addArg(n->getNickname())
					 .addArg(n->getIdentname())
					 .addArg(n->getHostname())
					 .addArg("*")
					 .addArg(n->getRealname()));
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

	CacaImage icon = n->getIcon();
	try
	{
		string buf = icon.getIRCBuffer(0, 10);
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
	string targets = message.getArg(0);
	const char * delim=",";
	string target = stringtok(targets, delim);

	while (target!=""){
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
		}
		target=stringtok(targets, delim);
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
		case 'm':
			for(size_t i = 0; i < sizeof commands / sizeof *commands; ++i)
				user->send(Message(RPL_STATSCOMMANDS).setSender(this)
						                     .setReceiver(user)
								     .addArg(commands[i].cmd)
								     .addArg(t2s(commands[i].count))
								     .addArg("0"));
			break;
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
		default:
			arg = "*";
			notice(user, "m (commands) - List all IRC commands");
			notice(user, "p (protocols) - List all protocols");
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
	if(message.countArgs() > 0)
	{
		string arg = message.getArg(0);
		switch(arg[0])
		{
			case 'a':
			{
				/* XXX Probably not needed. */
				message.rebuildWithQuotes();
				if(message.countArgs() < 2)
				{
					notice(user, "Usage: /MAP add PROTO USERNAME PASSWD [OPTIONS] [CHANNEL]");
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

				vector<im::Protocol::Option> options = proto.getOptions();
				if(message.countArgs() < 4)
				{
					string s;
					FOREACH(vector<im::Protocol::Option>, options, it)
					{
						if(!s.empty()) s += " ";
						s += "[-";
						switch(it->getType())
						{
							case PURPLE_PREF_BOOLEAN:
								s += "[!]" + it->getName();
								break;
							case PURPLE_PREF_STRING:
								s += it->getName() + " value";
								break;
							case PURPLE_PREF_INT:
								s += it->getName() + " int";
								break;
							default:
								break;
						}
						s += "]";
					}
					notice(user, "Usage: /MAP add " + proto.getID() + " USERNAME PASSWD " + s + " [CHANNEL]");
					return;
				}
				string username, password, channel;
				for(size_t i = 2; i < message.countArgs(); ++i)
				{
					string s = message.getArg(i);
					if(s[0] == '-')
					{
						size_t name_pos = 1;
						string value = "true";
						if(s[1] == '!')
						{
							value = "false";
							name_pos++;
						}

						vector<im::Protocol::Option>::iterator it = std::find(options.begin(), options.end(), s.substr(name_pos));
						if(it == options.end())
						{
							notice(user, "Error: Option '" + s + "' is unknown");
							return;
						}
						if(it->getType() == PURPLE_PREF_BOOLEAN)
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
						it->setValue(value);
						b_log[W_ERR] << it->getName() << "=" << value;
					}
					else if(username.empty())
						username = s;
					else if(password.empty())
						password = s;
					else if(channel.empty())
					{
						channel = s;
						if(!Channel::isStatusChannel(channel))
						{
							notice(user, "Error: Status channel must start with '&'");
							return;
						}
					}
				}

				added_account = im->addAccount(proto, username, password, options);
				added_account.setStatusChannel(channel);
				added_account.createStatusChannel();

				break;
			}
			case 'd':
			case 'r':
			{
				if(message.countArgs() != 2)
				{
					notice(user, "Usage: /MAP rem NAME");
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
			case 'h':
				notice(user,"a, add: add ACCOUNT to your accounts");
				notice(user,"r, rem: remove ACCOUNT from your accounts");
			default:
				notice(user,"Usage: /MAP [add PROTO USERNAME PASSWD [CHANNEL] [options] ] | [rem NAME] | [help]");
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
		string name;

		if(++next == accounts.end())
			name = "`-";
		else
			name = "|-";

		name += it->second.getServername();
		if(it->second == added_account)
			name += " (added)";
		else if(it->second.isConnecting())
			name += " (connecting)";
		else if(!it->second.isConnected())
			name += " (disconnected)";

		user->send(Message(RPL_MAP).setSender(this)
					   .setReceiver(user)
					   .addArg(name));
	}
	user->send(Message(RPL_MAPEND).setSender(this)
				      .setReceiver(user)
				      .addArg("End of /MAP"));

}

/** JOIN channame */
void IRC::m_join(Message message)
{
	string names = message.getArg(0);
	string channame;
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
					if(account.isConnecting())
						account.enqueueChannelJoin(convname);
					else
						user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
										     .setReceiver(user)
										     .addArg(channame)
										     .addArg("No such channel"));
					continue;
				}

				if(!account.joinChat(convname))
				{
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
				}


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

/** INVITE nick chan */
void IRC::m_invite(Message message)
{
	Channel* chan = getChannel(message.getArg(1));
	if(!chan)
	{
		user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(1))
						     .addArg("No such channel"));
		return;
	}

	if(chan->isStatusChannel())
	{
		/* Add a buddy */
		string acc = message.getArg(0);
		string username = stringtok(acc, ":");
		im::Account account;
		if(acc.empty())
			account = im->getAccountFromChannel(chan->getName());
		else
			account = im->getAccount(acc);
		if(!account.isValid())
		{
			user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
							     .setReceiver(user)
							     .addArg(message.getArg(1))
							     .addArg("No such channel"));
			return;
		}
		account.addBuddy(username, "minbif");
		user->send(Message(RPL_INVITING).setSender(this)
				                .setReceiver(user)
						.addArg(username)
						.addArg(chan->getName()));
	}
	else if(chan->isRemoteChannel())
	{
		/* \todo TODO implement /invite on a remote channel */
		ConversationChannel* cchan = dynamic_cast<ConversationChannel*>(chan);
		string buddy = message.getArg(0);

		cchan->invite(buddy, "");
		user->send(Message(RPL_INVITING).setSender(this)
				                .setReceiver(user)
						.addArg(buddy)
						.addArg(chan->getName()));
	}
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

	if(chan->isStatusChannel())
	{
		Buddy* buddy = dynamic_cast<Buddy*>(chanuser->getNick());
		if(!buddy)
		{
			user->send(Message(ERR_NOPRIVILEGES).setSender(this)
					                    .setReceiver(user)
							    .addArg("Permission denied: you can only kick a buddy"));
			return;
		}

		RemoteServer* rt = dynamic_cast<RemoteServer*>(buddy->getServer());
		if(!rt)
		{
			notice(user, chanuser->getName() + " is not on a remote server");
			return;
		}
		string reason = "Removed from buddy list";
		if(message.countArgs() > 2 && message.getArg(2).empty() == false)
			reason += ": " + message.getArg(2);

		buddy->kicked(chan, user_chanuser, reason);
		rt->getAccount().removeBuddy(buddy->getBuddy());
	}
	else if(chan->isRemoteChannel())
	{
		/* \todo TODO implement /kick on a remote channel */
	}
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

	if(getNick(message.getArg(1)))
	{
		user->send(Message(ERR_NICKNAMEINUSE).setSender(this)
				                     .setReceiver(user)
						     .addArg(message.getArg(1))
						     .addArg("Nickname is already in use"));
		return;
	}

	user->send(Message(MSG_NICK).setSender(buddy)
				    .addArg(message.getArg(1)));

	users.erase(buddy->getNickname());
	buddy->setNickname(message.getArg(1));
	addNick(buddy);
	buddy->getBuddy().setAlias(message.getArg(1));
}

}; /* namespace irc */
