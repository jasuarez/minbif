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

#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <libpurple/purple.h>

#include "minbif.h"
#include "version.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "im/im.h"
#include "server_poll/poll.h"

/** This is a derived class from ConfigItem whose represent an IP address item */
class ConfigItem_ipaddr : public ConfigItem
{
public:
	ConfigItem_ipaddr(std::string _label, std::string _description, std::string def_value = "",
		TCallBack cb = 0, MyConfig* _config = 0, ConfigSection* _parent = 0)
		: ConfigItem(_label, _description, def_value, cb, _config, _parent)
		{}

	virtual ConfigItem* Clone() const
	{
		return new ConfigItem_ipaddr(Label(), Description(), DefValue(), CallBack(), GetConfig(), Parent());
	}

	/** We return a string form of this integer */
	virtual std::string String() const { return value; }

	virtual bool SetValue(std::string s)
	{
		if(!is_ip(s.c_str()))
			return false;
		value = s;
		return true;
	}

	std::string ValueType() const
	{
		return "ip address";
	}

private:
	string value;
};

/** This is a derived class from ConfigItem whose represent an integer range item */
class ConfigItem_intrange : public ConfigItem_int
{
public:
	ConfigItem_intrange(std::string _label, std::string _description, int _min = INT_MIN, int _max = INT_MAX, std::string def_value = "",
		TCallBack cb = 0, MyConfig* _config = 0, ConfigSection* _parent = 0)
		: ConfigItem_int(_label, _description, _min, _max, def_value, cb, _config, _parent)
		{}

	virtual ConfigItem* Clone() const
	{
		return new ConfigItem_intrange(Label(), Description(), min, max, DefValue(), CallBack(), GetConfig(), Parent());
	}

	/** We return a string form of this integer */
	virtual std::string String() const { std::ostringstream oss; oss << value_min << "-" << value_max; return oss.str(); }
	virtual int MinInteger() const { return value_min; }
	virtual int MaxInteger() const { return value_max; }

	virtual bool SetValue(std::string s)
	{
		string min_s, max_s;
		bool on_min = true;

		for(std::string::const_iterator it = s.begin(); it != s.end(); ++it)
		{
			if(isdigit(*it))
			{
				if(on_min) min_s += *it;
				else max_s += *it;
			}
			else if(on_min && (*it == ':' || *it == '-'))
				on_min = false;
			else
				return false;

		}
		std::istringstream(min_s) >> value_min;
		std::istringstream(max_s) >> value_max;
		return (value_min >= min && value_max >= value_min && value_max <= max);
	}

	std::string ValueType() const
	{
		std::ostringstream off, off2;
		std::string in, ax;
		off << min;
		in = off.str();
		off2 << max;
		ax = off2.str();
		return "integer range (between " + in + " and " + ax + ")";
	}

private:
	int value_min, value_max;
};


Minbif::Minbif()
	: loop(0), server_poll(0)
{
	ConfigSection* section;
	section = conf.AddSection("path", "Path information", false);
	section->AddItem(new ConfigItem_string("users", "Users directory"));

	section = conf.AddSection("irc", "Server information", false);
	section->AddItem(new ConfigItem_string("hostname", "Server hostname", " "));
	section->AddItem(new ConfigItem_int("type", "Type of daemon", 0, 2, "0"));
	section->AddItem(new ConfigItem_int("ping", "Ping frequence (s)", 0, 65535, "60"));

	section = section->AddSection("daemon", "Daemon information", true);
	section->AddItem(new ConfigItem_ipaddr("bind", "IP address to listen on"));
	section->AddItem(new ConfigItem_int("port", "Port to listen on", 1, 65535), true);
	section->AddItem(new ConfigItem_bool("background", "Start minbif in background", "true"));

	section = conf.AddSection("file_transfers", "File transfers parameters", false);
	section->AddItem(new ConfigItem_bool("enabled", "Enable file transfers", "true"));
	section->AddItem(new ConfigItem_bool("dcc", "Send files to IRC user with DCC", "true"));
	section->AddItem(new ConfigItem_ipaddr("bind", "IP address to listen on for DCC", "0.0.0.0"));
	section->AddItem(new ConfigItem_intrange("port_range", "Port range to listen on for DCC", 1024, 65535, "1024-65535"));

	section = conf.AddSection("logging", "Log information", false);
	section->AddItem(new ConfigItem_string("level", "Logging level"));
	section->AddItem(new ConfigItem_bool("to_syslog", "Log error and warnings to syslog"));

}

Minbif::~Minbif()
{
	delete server_poll;
}

static void sighandler(int r)
{
	switch(r)
	{
		case SIGCHLD:
		{
			pid_t pid;
			int st;
			while((pid = waitpid(0, &st, WNOHANG)) > 0)
				;
			break;
		}
		case SIGPIPE:
			break;
		case SIGTERM:
			/* TODO implement a handler here. */
		default:
			raise(r);
	}
}

int Minbif::main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "Syntax: " << argv[0] << " config_file" << std::endl;
		return EXIT_FAILURE;
	}

	if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
	{
		std::cout << MINBIF_VERSION << " (Build " __DATE__ " " __TIME__ ") © 2009 Romain Bignon" << std::endl;
		return EXIT_SUCCESS;
	}

	try
	{
		struct rlimit rlim;
		if(!getrlimit(RLIMIT_CORE, &rlim) && rlim.rlim_cur != RLIM_INFINITY)
		{
			rlim.rlim_cur = RLIM_INFINITY;
			rlim.rlim_max = RLIM_INFINITY;
			setrlimit(RLIMIT_CORE, &rlim);
		}

		if(!conf.Load(argv[1]))
		{
			b_log[W_ERR] << "Unable to load configuration, exiting..";
			return EXIT_FAILURE;
		}
		b_log.SetLoggedFlags(conf.GetSection("logging")->GetItem("level")->String(), conf.GetSection("logging")->GetItem("to_syslog")->Boolean());

		/* Set users directory path and if I have rights to write in. */
		im::IM::setPath(conf.GetSection("path")->GetItem("users")->String());

		server_poll = ServerPoll::build((ServerPoll::poll_type_t)conf.GetSection("irc")->GetItem("type")->Integer(),
				                this);
		b_log.setServerPoll(server_poll);

		struct sigaction sig, old;
		memset( &sig, 0, sizeof( sig ) );
		sig.sa_handler = sighandler;
		sigaction( SIGCHLD, &sig, &old );
		sigaction( SIGPIPE, &sig, &old );
		sig.sa_flags = SA_RESETHAND;
		sigaction( SIGINT,  &sig, &old );
		sigaction( SIGILL,  &sig, &old );
		sigaction( SIGBUS,  &sig, &old );
		sigaction( SIGFPE,  &sig, &old );
		sigaction( SIGSEGV, &sig, &old );
		sigaction( SIGTERM, &sig, &old );
		sigaction( SIGQUIT, &sig, &old );
		sigaction( SIGXCPU, &sig, &old );

		g_thread_init(NULL);
		loop = g_main_new(FALSE);
		g_main_run(loop);

		return EXIT_SUCCESS;
	}
	catch(MyConfig::error_exc &e)
	{
		b_log[W_ERR] << "Error while loading:";
		b_log[W_ERR] << e.Reason();
	}
	catch(im::IMError &e)
	{
		b_log[W_ERR] << "Unable to load IM settings";
	}
	catch(ServerPollError &e)
	{
		b_log[W_ERR] << "Unable to load the server poll.";
	}

	return EXIT_FAILURE;
}

void Minbif::quit()
{
	g_main_quit(loop);
}

int main(int argc, char** argv)
{
	Minbif minbif;
	return minbif.main(argc, argv);
}
