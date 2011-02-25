/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2011 Romain Bignon, Marc Dequènes (Duck)
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

#include <cerrno>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/resource.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>

#include "minbif.h"
#include "sighandler.h"
#include "version.h"
#include "log.h"
#include "util.h"
#include "im/im.h"
#include "server_poll/poll.h"

const char* infotxt[] = {
	MINBIF_VERSION,
	strdup(MINBIF_BUILD.c_str()), /* don't care if we don't free it as it can be used until exit. */
	"",
	"Copyright(C) 2009-2011 Romain Bignon",
	"This program is free software; you can redistribute it and/or modify",
	"it under the terms of the GNU General Public License as published by",
	"the Free Software Foundation, version 2 of the License.",
	NULL
};

Minbif::Minbif()
	: loop(NULL),
	  server_poll(0)
{
	ConfigSection* section;
	ConfigSection* sub;

	section = conf.AddSection("path", "Path information", MyConfig::NORMAL);
	section->AddItem(new ConfigItem_string("users", "Users directory"));
	section->AddItem(new ConfigItem_string("motd", "Path to motd", " "));

	section = conf.AddSection("irc", "Server information", MyConfig::NORMAL);
	section->AddItem(new ConfigItem_string("hostname", "Server hostname", " "));
	section->AddItem(new ConfigItem_string("password", "Global server password", " "));
	section->AddItem(new ConfigItem_int("type", "Type of daemon", 0, 2, "0"));
	section->AddItem(new ConfigItem_int("ping", "Ping frequence (s)", 0, 65535, "60"));
	section->AddItem(new ConfigItem_string("buddy_icons_url", "URL to display in /WHOIS to get a buddy icon", " "));

	sub = section->AddSection("inetd", "Inetd information", MyConfig::OPTIONAL);
	add_server_block_common_params(sub);

	sub = section->AddSection("daemon", "Daemon information", MyConfig::OPTIONAL);
	sub->AddItem(new ConfigItem_string("bind", "IP address to listen on"));
	sub->AddItem(new ConfigItem_int("port", "Port to listen on", 1, 65535), true);
	sub->AddItem(new ConfigItem_bool("background", "Start minbif in background", "true"));
	sub->AddItem(new ConfigItem_int("maxcon", "Maximum simultaneous connections", 0, 65535, "0"));
	add_server_block_common_params(sub);

	sub = section->AddSection("oper", "Define an IRC operator", MyConfig::MULTIPLE);
	sub->AddItem(new ConfigItem_string("login", "Nickname of IRC operator"), true);
	sub->AddItem(new ConfigItem_string("password", "IRC operator password"));
	sub->AddItem(new ConfigItem_string("email", "IRC operator email address", "*@*"));

	section = conf.AddSection("aaa", "Authentication, Authorization and Accounting", MyConfig::OPTIONAL);
	section->AddItem(new ConfigItem_bool("use_local", "Use local database to authenticate users", "true"));
#ifdef HAVE_PAM
	section->AddItem(new ConfigItem_bool("use_pam", "Use PAM mechanisms to authenticate/authorize users", "false"));
	section->AddItem(new ConfigItem_bool("pam_setuid", "Child process setuid with the pam user (needs root and pam auth)", "false"));
#endif
	section->AddItem(new ConfigItem_bool("use_connection", "Use connection information to authenticate/authorize users", "false"));

	section = conf.AddSection("file_transfers", "File transfers parameters", MyConfig::OPTIONAL);
	section->AddItem(new ConfigItem_bool("enabled", "Enable file transfers", "true"));
	section->AddItem(new ConfigItem_bool("dcc", "Send files to IRC user with DCC", "true"));
	section->AddItem(new ConfigItem_string("dcc_own_ip", "Force minbif to always send DCC requests from a particular IP address", " "));
	section->AddItem(new ConfigItem_intrange("port_range", "Port range to listen on for DCC", 1024, 65535, "1024-65535"));

	section = conf.AddSection("logging", "Log information", MyConfig::NORMAL);
	section->AddItem(new ConfigItem_string("level", "Logging level"));
	section->AddItem(new ConfigItem_bool("to_syslog", "Log error and warnings to syslog"));
	section->AddItem(new ConfigItem_bool("conv_logs", "Enable conversation logging", "false"));

}

void Minbif::add_server_block_common_params(ConfigSection* section)
{
	section->AddItem(new ConfigItem_string("security", "none/tls/starttls/starttls-mandatory", "none"));
#ifdef HAVE_TLS
	ConfigSection* sub = section->AddSection("tls", "TLS information", MyConfig::OPTIONAL);
	sub->AddItem(new ConfigItem_string("trust_file", "CA certificate file for TLS", " "));
	sub->AddItem(new ConfigItem_string("crl_file", "CA certificate file for TLS", " "));
	sub->AddItem(new ConfigItem_string("cert_file", "Server certificate file for TLS"));
	sub->AddItem(new ConfigItem_string("key_file", "Server key file for TLS"));
	sub->AddItem(new ConfigItem_string("priority", "Priority list for ciphers, exchange methods, macs and compression methods", "NORMAL"));
#endif
}

Minbif::~Minbif()
{
	delete server_poll;
	remove_pidfile();
}

void Minbif::remove_pidfile(void)
{
	if(pidfile.empty())
		return;
	std::ifstream fp(pidfile.c_str());
	if(!fp)
		return;
	int i;
	fp >> i;
	if(i != getpid())
		return;

	unlink(pidfile.c_str());

}

void Minbif::usage(int argc, char** argv)
{
	std::cerr << "Usage: " << argv[0] << " [OPTIONS]... CONFIG_PATH" << std::endl << std::endl;
	std::cerr << "Options:" << std::endl;
	std::cerr << "  -h, --help             Display this notice" << std::endl;
	std::cerr << "  -v, --version          Version of minbif" << std::endl;
	std::cerr << "  -m, --mode=MODE        Mode to run (MODE is a number)" << std::endl;
	std::cerr << "  -p, --pidfile=PIDFILE  Path to pid file" << std::endl;
}

void Minbif::version(void)
{
	for(size_t i = 0; infotxt[i] != NULL; ++i)
		std::cout << infotxt[i] << std::endl;
}

int Minbif::main(int argc, char** argv)
{
	static struct option long_options[] =
	{
		{ "pidfile",       1, NULL, 'p' },
		{ "help",          0, NULL, 'h' },
		{ "version",       0, NULL, 'v' },
		{ "mode",          1, NULL, 'm' },
		{ NULL,            0, NULL, 0   }
	};
	int option_index = 0, c;
	int mode = -1;
	while((c = getopt_long(argc, argv, "m:p:hv", long_options, &option_index)) != -1)
		switch(c)
		{
		case 'm':
			mode = atoi(optarg);
			break;
		case 'h':
			usage(argc, argv);
			return EXIT_SUCCESS;
			break;
		case 'v':
			version();
			return EXIT_SUCCESS;
			break;
		case 'p':
		{
			std::ifstream fi(optarg);
			if(fi)
			{
				std::cerr << "It seems that minbif is already launched. Perhaps try to erase file " << optarg << std::endl;
				fi.close();
				return EXIT_FAILURE;
			}
			pidfile = optarg;
			break;
		}
		default:
			return EXIT_FAILURE;
			break;
		}

	if(optind >= argc)
	{
		std::cerr << argv[0] << ": wrong argument count" << std::endl;
		usage(argc, argv);
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

		if(!conf.Load(argv[optind]))
		{
			b_log[W_ERR] << "Unable to load configuration, exiting..";
			return EXIT_FAILURE;
		}
		b_log.setLoggedFlags(conf.GetSection("logging")->GetItem("level")->String(), conf.GetSection("logging")->GetItem("to_syslog")->Boolean());

		/* Set users directory path and if I have rights to write in. */
		im::IM::setPath(conf.GetSection("path")->GetItem("users")->String());

		if (mode < 0)
			mode = conf.GetSection("irc")->GetItem("type")->Integer();

		server_poll = ServerPoll::build((ServerPoll::poll_type_t)mode, this);
		b_log.setServerPoll(server_poll);

		if(!pidfile.empty())
		{
			std::ofstream fo(pidfile.c_str());
			if(!fo)
			{
				std::cerr << "Unable to create file '" << pidfile << "': " << strerror(errno) << std::endl;
				return EXIT_FAILURE;
			}
			fo << getpid() << std::endl;
			fo.close();
		}
		sighandler.setApplication(this);

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
		b_log[W_ERR] << "Unable to load IM settings: " << e.Reason();
	}
	catch(ServerPollError &e)
	{
		b_log[W_ERR] << "Unable to load the server poll.";
	}

	return EXIT_FAILURE;
}

void Minbif::rehash()
{
	if(!conf.Load())
	{
		b_log[W_ERR] << "Unable to load configuration, exiting..";
		quit();
	}
	b_log.setLoggedFlags(conf.GetSection("logging")->GetItem("level")->String(), conf.GetSection("logging")->GetItem("to_syslog")->Boolean());
	if(server_poll)
		server_poll->rehash();
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
