/*
 * Minbif - IRC instant messaging gateway
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

#include <cassert>

#include "irc/settings.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "irc/channel.h"
#include "server_poll/poll.h"
#include "core/version.h"
#include "core/util.h"

namespace irc {

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
		{ "send_delay",                true,  new SettingSendDelay(this, im) },
		{ "voiced_buddies",            true,  new SettingVoicedBuddies(this, im) },
		{ "server_aliases",            true,  new SettingServerAliases(this, im) },
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
			for(size_t i = 0; commands[i].cmd != NULL; ++i)
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
		case 'P':
		{
			map<string, im::Plugin> plugins = im->getPluginsList();
			if (message.countArgs() < 2)
			{
				/* No argument */
				for (map<string, im::Plugin>::iterator it = plugins.begin();
				     it != plugins.end(); ++it)
				{
					im::Plugin plug = it->second;
					string s = "[";
					if (plug.isLoaded())
						s += "+";
					else
						s += " ";
					s += "] ";
					s += plug.getID();
					s += " (";
					s += plug.getName() + ": " + plug.getSummary();
					s += ")";
					notice(user, s);
				}
				break;
			}
			map<string, im::Plugin>::iterator it = plugins.find(message.getArg(1));
			if (it == plugins.end())
			{
				notice(user, message.getArg(1) + ": No such plugin");
				break;
			}
			im::Plugin plug = it->second;
			if (message.countArgs() < 3)
			{
				/* Only give a plugin name. */
				notice(user, "-- Information about " + plug.getID());
				notice(user, "Description: ");
				notice(user, plug.getDescription());
				notice(user, string("Loaded: ") + (plug.isLoaded() ? "true" : "false"));
				break;
			}
			string command = message.getArg(2);
			switch(command[0])
			{
				case 'l':
					try {
						plug.load();
						notice(user, "Plugin '" + plug.getID() + "' loaded.");
					} catch (im::Plugin::LoadError &e) {
						notice(user, e.Reason());
					}
					break;
				case 'u':
					try {
						plug.unload();
						notice(user, "Plugin '" + plug.getID() + "' unloaded.");
					} catch(im::Plugin::LoadError &e) {
						notice(user, e.Reason());
					}
					break;
				default:
					notice(user, "No suck command: " + command);
					break;
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
			notice(user, "P (plugins) - List, load and configure plugins");
			notice(user, "u (uptime) - Display the server uptime");
			break;
	}
	user->send(Message(RPL_ENDOFSTATS).setSender(this)
					  .setReceiver(user)
					  .addArg(arg)
					  .addArg("End of /STATS report"));
}

} /* ns irc */
