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

#include <iostream>
#include <syslog.h>
#include <sys/time.h>

#include "util.h"
#include "log.h"
#include "server_poll/poll.h"

Log b_log;

static struct
{
	uint32_t flag;
	int level;
	const char* s;
	bool sys_log;
	bool propagate_to_user;
} all_flags[] =
{
	{ W_SNO,        LOG_NOTICE,  ""       , false,  true  },
	{ W_DEBUG,      LOG_DEBUG,   "DEBUG"  , false,  true  },
	{ W_PARSE,      LOG_DEBUG,   "PARSE"  , false,  true  },
	{ W_PURPLE,     LOG_DEBUG,   "PURPLE" , false,  true  },
	{ W_DESYNCH,    LOG_WARNING, "DESYNCH", false,  true  },
	{ W_WARNING,    LOG_WARNING, "WARNING", false,  true  },
	{ W_ERR,        LOG_ERR,     "ERR"    , true,   true  },
	{ W_INFO,       LOG_NOTICE,  "INFO"   , false,  true  },
	{ W_SOCK,	LOG_DEBUG,   "SOCK"   , false,  false },
};

Log::flux::~flux()
{
	int i;

	if(!(flag & b_log.getLoggedFlags()))
		return;

	for(i = (sizeof all_flags / sizeof *all_flags) - 1; i >= 0 && !(flag & all_flags[i].flag); --i)
		;

	if(i < 0)
		syslog(LOG_WARNING, "[SYSLOG] (%X) Unable to find how to log this message: %s", (uint32_t)flag, str.c_str());
	else
	{
		if(all_flags[i].sys_log && b_log.toSyslog())
			syslog(all_flags[i].level, "[%s] %s", all_flags[i].s, str.c_str());

                if (!all_flags[i].propagate_to_user)
			return;

		struct timeval t;
		gettimeofday(&t, NULL);

		string category;
		if(flag & W_SNO)
			category = "*** Notice -- ";
		else
			category = string("[") + all_flags[i].s + "] ";

		if(b_log.getServerPoll())
			b_log.getServerPoll()->log(flag, category + str);

	}
}

Log::Log()
	: logged_flags(DEFAULT_LOGGED_FLAGS),
	  poll(NULL)
{
	openlog("minbif", LOG_CONS, LOG_DAEMON);
}

Log::~Log()
{
	closelog();
}

std::string Log::formatLoggedFlags() const
{
	std::string s;
	int i;
	for(i = (sizeof all_flags / sizeof *all_flags) - 1; i >= 0; --i)
		if(logged_flags & all_flags[i].flag)
		{
			if(!s.empty()) s.append(" ");
			s += all_flags[i].s;
		}
	return s;
}

void Log::setLoggedFlags(std::string s, bool _to_syslog)
{
	std::string token;

	to_syslog = _to_syslog;
	logged_flags = 0;

	while((token = stringtok(s, " ")).empty() == false)
	{
		int i;
		if(token == "ALL")
		{
			logged_flags = (uint32_t) -1;
			break;
		}

		for(i = (sizeof all_flags / sizeof *all_flags) - 1; i >= 0 && (token != all_flags[i].s); --i)
			;

		if(i >= 0)
			logged_flags |= all_flags[i].flag;
	}
}
