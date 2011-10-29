/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2011 Romain Bignon
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
#include <fnmatch.h>

#include "core/caca_image.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "irc/buddy.h"
#include "irc/channel.h"

namespace irc {

/** WHO */
void IRC::m_who(Message message)
{
	string arg;
	Channel* chan = NULL;
	enum
	{
		WHO_STATUS = 1 << 0
	};
#define fset(v, b, f) v = b ? v|f : v&(~f)
	int flags = 0;

	if(message.countArgs() > 0)
	{
		vector<string> args = message.getArgs();
		for(vector<string>::iterator it = args.begin(); it != args.end(); ++it)
		{
			bool add = true;
			if (!strchr("+-", (*it)[0]))
			{
				arg = *it;
				continue;
			}

			for (string::iterator c = it->begin(); c != it->end(); ++c)
				switch (*c) {
					case '+':
						add = true;
						break;
					case '-':
						add = false;
						break;
					case 's':
						fset(flags, add, WHO_STATUS);
						break;
				}
		}
	}
#undef fset

	if(arg.empty() || !Channel::isChanName(arg) || (chan = getChannel(arg)))
		for(std::map<string, Nick*>::iterator it = users.begin(); it != users.end(); ++it)
		{
			Nick* n = it->second;
			string channame = "*";
			if(arg.empty() || arg == "*" || arg == "0" || n->getServer()->getServerName().find(arg) != string::npos ||
			   !fnmatch(arg.c_str(), n->getNickname().c_str(), FNM_NOESCAPE|FNM_CASEFOLD))
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
							.addArg("0 " + (flags & WHO_STATUS ? n->getStatusMessage(true)
							                                   : n->getRealName())));
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

	string status = n->getStatusMessage();
	if (!status.empty())
		user->send(Message(RPL_WHOISACTUALLY).setSender(this)
						     .setReceiver(user)
						     .addArg(n->getNickname())
						     .addArg(status));

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
				/* purple_url_decode() returns a static buffer, no free needed. */
				string convname = purple_url_decode(stringtok(accid, ":").c_str());
				string param = purple_url_decode(stringtok(parameters, ",").c_str());
				if(accid.empty() || convname.empty())
				{
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
					continue;
				}
				im::Account account = im->getAccount(accid);
				if(!account.isValid())
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
				else if(!account.isConnected())
					account.enqueueChannelJoin(convname);
				else if(!account.joinChat(convname, param))
					user->send(Message(ERR_NOSUCHCHANNEL).setSender(this)
									     .setReceiver(user)
									     .addArg(channame)
									     .addArg("No such channel"));
				else
					g_timeout_add(1000, g_callback_delete, new CallBack<IRC>(this, &IRC::check_channel_join, g_strdup(channame.c_str())));

#if 0
				/* wait the signal from libpurple to create the conversation channel. */
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
	Nick* n2 = NULL;
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

	if((n2 = getNick(message.getArg(1))) && n2 != n)
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
	buddy->getBuddy().setAlias(message.getArg(1), true);
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

} /* ns irc */
