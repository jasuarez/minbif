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
#include <sys/resource.h>
#include <libpurple/purple.h>

#include "minbif.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "im/im.h"
#include "server_poll/poll.h"

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

	section = conf.AddSection("logging", "Log information", false);
	section->AddItem(new ConfigItem_string("level", "Logging level"));
	section->AddItem(new ConfigItem_bool("to_syslog", "Log error and warnings to syslog"));
}

Minbif::~Minbif()
{
	delete server_poll;
}

int Minbif::main(int argc, char** argv)
{
	if(argc < 2)
	{
		b_log << "Syntax: " << argv[0] << " config_file";
		return EXIT_FAILURE;
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
