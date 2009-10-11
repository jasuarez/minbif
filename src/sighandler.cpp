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

#include <sys/wait.h>
#include <cstring>

#include "sighandler.h"
#include "minbif.h"
#include "callback.h"
#include "util.h"

SigHandler sighandler;

SigHandler::SigHandler()
	: app(NULL)
{

}

SigHandler::~SigHandler()
{

}

void SigHandler::setApplication(Minbif* app)
{
	this->app = app;

	struct sigaction sig, old;
	memset( &sig, 0, sizeof( sig ) );
	sig.sa_handler = &SigHandler::handler;
	sigaction(SIGCHLD, &sig, &old);
	sigaction(SIGPIPE, &sig, &old);
	sigaction(SIGHUP,  &sig, &old);
	sig.sa_flags = SA_RESETHAND;
	sigaction(SIGINT,  &sig, &old);
	sigaction(SIGILL,  &sig, &old);
	sigaction(SIGBUS,  &sig, &old);
	sigaction(SIGFPE,  &sig, &old);
	sigaction(SIGSEGV, &sig, &old);
	sigaction(SIGTERM, &sig, &old);
	sigaction(SIGQUIT, &sig, &old);
	sigaction(SIGXCPU, &sig, &old);
}

bool SigHandler::rehash(void*)
{
	app->rehash();
	return false;
}

bool SigHandler::quit(void*)
{
	app->quit();
	return false;
}

void SigHandler::handler(int r)
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
		case SIGHUP:
			g_timeout_add(0, g_callback_delete, new CallBack<SigHandler>(&sighandler, &SigHandler::rehash));
			break;
		case SIGTERM:
			g_timeout_add(0, g_callback_delete, new CallBack<SigHandler>(&sighandler, &SigHandler::quit));
			break;
		default:
			raise(r);
	}
}
