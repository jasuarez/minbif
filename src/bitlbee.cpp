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

#include <stdlib.h>
#include <libpurple/purple.h>

#include "bitlbee.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "server_poll/poll.h"

#if 0
static guint purple_wg_input_add(gint fd, PurpleInputCondition condition,
	PurpleInputFunction function, gpointer data)
{
	return 0;
}

static PurpleEventLoopUiOps eventloop_wg_ops = {
	/* timeout_add */    g_timeout_add,
	/* timeout_remove */ g_source_remove,
	/* input_add */      purple_wg_input_add,
	/* input_remove */   g_source_remove,
	/* input_get_error*/ NULL,
	/* timeout_add_seconds */ NULL,
	NULL, NULL, NULL
};
#endif

Bitlbee::Bitlbee()
	: loop(0), server_poll(0)
{
	ConfigSection* section;
	section = conf.AddSection("path", "Path information", false);
	section->AddItem(new ConfigItem_string("users", "Users directory"));

	section = conf.AddSection("irc", "Server information", false);
	section->AddItem(new ConfigItem_string("hostname", "Server hostname", " "));
	section->AddItem(new ConfigItem_string("command_chan", "Command channel name", "&bitlbee"));
	section->AddItem(new ConfigItem_int("type", "Type of daemon", 0, 2, "0"));
	section->AddItem(new ConfigItem_int("ping", "Ping frequence (s)", 1, 65535, "60"));

	section = conf.AddSection("logging", "Log informations", false);
	section->AddItem(new ConfigItem_string("level", "Logging level"));
	section->AddItem(new ConfigItem_bool("to_syslog", "Log error and warnings to syslog"));
}

Bitlbee::~Bitlbee()
{
	delete server_poll;
}

int Bitlbee::main(int argc, char** argv)
{
	if(argc < 2)
	{
		b_log << "Syntax: " << argv[0] << " config_file";
		return EXIT_FAILURE;
	}

	try
	{
		if(!conf.Load(argv[1]))
		{
			b_log[W_ERR] << "Unable to load configuration, exiting..";
			return EXIT_FAILURE;
		}
		b_log.SetLoggedFlags(conf.GetSection("logging")->GetItem("level")->String(), conf.GetSection("logging")->GetItem("to_syslog")->Boolean());

		server_poll = ServerPoll::build((ServerPoll::poll_type_t)conf.GetSection("irc")->GetItem("type")->Integer(),
				                this);

		loop = g_main_new(FALSE);
		g_main_run(loop);

#if 0
		purple_util_set_user_dir(conf.GetSection("path")->GetItem("users")->String().c_str());
		purple_eventloop_set_ui_ops(&eventloop_wg_ops);
		purple_core_init("bitlbee2");
#endif

		return EXIT_SUCCESS;
	}
	catch(MyConfig::error_exc &e)
	{
		b_log[W_ERR] << "Error while loading:";
		b_log[W_ERR] << e.Reason();
	}
	catch(ServerPollError &e)
	{
		b_log[W_ERR] << "Unable to load the server poll.";
	}

	return EXIT_FAILURE;
}

void Bitlbee::quit()
{
	g_main_quit(loop);
}

int main(int argc, char** argv)
{
	Bitlbee bitlbee;
	exit(bitlbee.main(argc, argv));
}
