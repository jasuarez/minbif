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

#include "irc/irc.h"
#include "irc/user.h"
#include "irc/status_channel.h"

namespace irc {

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
			Channel* chan = account.getStatusChannel();
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

bool IRC::m_map_registeradd(Message& message, im::Account& added_account, bool register_account)
{
	/* XXX Probably not needed. */
	message.rebuildWithQuotes();
	if(message.countArgs() < 2)
	{
		notice(user, "Usage: /MAP add PROTO USERNAME [OPTIONS]");
		notice(user, "To display available options: /MAP add PROTO");
		notice(user, "Use '/STATS p' to show all available protocols");
		return false;
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
		return false;
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
				case im::Protocol::Option::SERVER_ALIASES:
				case im::Protocol::Option::BOOL:
					s += "[!]" + it->second.getName();
					break;
				case im::Protocol::Option::ACCID:
				case im::Protocol::Option::STATUS_CHANNEL:
				case im::Protocol::Option::PASSWORD:
				case im::Protocol::Option::STR:
					s += it->second.getName() + " value";
					break;
				case im::Protocol::Option::STR_LIST:
				{
					string choices;
					vector<string> choices_vec = it->second.getChoices();
					for(vector<string>::iterator c = choices_vec.begin(); c != choices_vec.end(); ++c)
					{
						if (!choices.empty()) choices += "|";
						choices += *c;
					}
					s += it->second.getName() + " [" + choices + "]";
					break;
				}
				case im::Protocol::Option::INT:
					s += it->second.getName() + " int";
					break;
				case im::Protocol::Option::NONE:
					break;
			}
			s += "]";
		}
		notice(user, "Usage: /MAP add " + proto.getID() + " USERNAME " + s);
		return false;
	}
	string username;
	try {
		for(size_t i = 2; i < message.countArgs(); ++i)
		{
			string s = message.getArg(i);
			if(username.empty())
				username = purple_url_decode(s.c_str()); /* static buffer, does not need free */
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
					return false;
				}
				switch(it->second.getType())
				{
					case im::Protocol::Option::BOOL:
					case im::Protocol::Option::SERVER_ALIASES:
						/* No input value needed, already got above */
						break;
					default:
						if(i+1 < message.countArgs())
							value = message.getArg(++i);
						else
						{
							notice(user, "Error: Option '" + s + "' needs a value");
							return false;
						}
				}
				it->second.setValue(value);
			}
			else if (options["password"].getValue().empty())
				options["password"].setValue(s);
			else
				options["status_channel"].setValue(s);
		}

		added_account = im->addAccount(proto, username, options, register_account);
	} catch (im::Protocol::OptionError& e) {
		notice(user, "Error: " + e.Reason());
		return false;
	}

	return true;
}

bool IRC::m_map_add(Message& message, im::Account& added_account)
{
	return m_map_registeradd(message, added_account, false);
}

bool IRC::m_map_register(Message& message, im::Account& added_account)
{
	return m_map_registeradd(message, added_account, true);
}

bool IRC::m_map_edit(Message& message, im::Account&)
{
	if(message.countArgs() < 2)
	{
		notice(user, "Usage: /MAP edit ACCOUNT [KEY [VALUE]]");
		return false;
	}
	im::Account account = im->getAccount(message.getArg(1));
	if(!account.isValid())
	{
		notice(user, "Error: Account " + message.getArg(1) + " is unknown");
		return false;
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
		return false;
	}
	im::Protocol::Options::iterator option;
	for(option = options.begin();
	    option != options.end() && option->second.getName() != message.getArg(2);
	    ++option)
		;

	if(option == options.end())
	{
		notice(user, "Key " + message.getArg(2) + " does not exist.");
		return false;
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
			return false;
		}
		/* TODO check if value is an integer if option is an integer */
		option->second.setValue(value);
		if(option->second.getType() == im::Protocol::Option::INT)
			notice(user, option->second.getName() + " = " + t2s(option->second.getValueInt()));
		else
			notice(user, option->second.getName() + " = " + option->second.getValue());

		try {
			account.setOptions(options);
		} catch(im::Protocol::OptionError& e) {
			notice(user, "Error: " + e.Reason());
		}
	}
	return false;
}

bool IRC::m_map_delete(Message& message, im::Account&)
{
	if(message.countArgs() != 2)
	{
		notice(user, "Usage: /MAP delete ACCOUNT");
		return false;
	}
	im::Account account = im->getAccount(message.getArg(1));
	if (!account.isValid())
	{
		notice(user, "Error: Account " + message.getArg(1) + " is unknown");
		return false;
	}
	notice (user, "Removing account "+account.getUsername());
	im->delAccount(account);

	return true;
}

bool IRC::m_map_command(Message& message, im::Account&)
{
	if(message.countArgs() < 2)
	{
		notice(user, "Usage: /MAP cmd ACCOUNT [command]");
		return false;
	}
	im::Account account = im->getAccount(message.getArg(1));
	if(!account.isValid())
	{
		notice(user, "Error: Account " + message.getArg(1) + " is unknown");
		return false;
	}
	if(!account.isConnected())
	{
		notice(user, "Error: Account " + message.getArg(1) + " is not connected");
		return false;
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

	return false;
}

bool IRC::m_map_help(Message& message, im::Account&)
{
	notice(user,"add PROTO USERNAME PASSWD [CHANNEL] [options]   add an account");
	notice(user,"reg PROTO USERNAME PASSWD [CHANNEL] [options]   add an account and register it on server");
	notice(user,"edit ACCOUNT [KEY [VALUE]]                      edit an account");
	notice(user,"cmd ACCOUNT [COMMAND]                           run a command on an account");
	notice(user,"delete ACCOUNT                                  remove ACCOUNT from your accounts");
	notice(user,"help                                            display this message");
	notice(user,"Usage: /MAP add|register|edit|command|delete|help [...]");

	return false;
}

/** MAP */
void IRC::m_map(Message message)
{
	im::Account added_account;
	if(message.countArgs() > 0)
	{
		string arg = message.getArg(0);

		static const struct {
			const char *name;
			bool (IRC::*func) (Message& message, im::Account& account);
		} commands[] = {
			{ "register",    &IRC::m_map_register },
			{ "add",         &IRC::m_map_add },
			{ "edit",        &IRC::m_map_edit },
			{ "delete",      &IRC::m_map_delete },
			{ "command",     &IRC::m_map_command },
			{ "cmd",         &IRC::m_map_command },
			{ "help",        &IRC::m_map_help },
		};
		size_t i;

		for (i = 0;
		     i < (sizeof(commands)/sizeof(*commands)) && string(commands[i].name).find(arg) != 0;
		     ++i)
			;

		if (i >= (sizeof(commands)/sizeof(*commands)))
		{
			m_map_help(message, added_account);
			return;
		}

		if (!(this->*commands[i].func)(message, added_account))
			return;
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

} /* ns irc */
