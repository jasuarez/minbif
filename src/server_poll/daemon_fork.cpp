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
#include <cassert>
#include <cstring>
#include <cerrno>
#include <glib/gmain.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "daemon_fork.h"
#include "../config.h"
#include "../irc/irc.h"
#include "../irc/user.h"
#include "../config.h"
#include "../callback.h"
#include "../log.h"
#include "../minbif.h"
#include "../util.h"

DaemonForkServerPoll::DaemonForkServerPoll(Minbif* application)
	: ServerPoll(application),
	  irc(NULL),
	  read_cb(NULL),
	  stop_cb(NULL)
{
	std::vector<ConfigSection*> sections = conf.GetSection("irc")->GetSectionClones("daemon");
	if(sections.empty())
	{
		b_log[W_ERR] << "Missing section irc/daemon";
		throw ServerPollError();
	}

	ConfigSection* section = sections[0];
	const char* bind_addr = section->GetItem("bind")->String().c_str();
	uint16_t port = (uint16_t)section->GetItem("port")->Integer();

	static struct sockaddr_in fsocket;
	fsocket.sin_family = AF_INET;
	fsocket.sin_addr.s_addr = inet_addr(bind_addr);
	fsocket.sin_port = htons(port);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
	{
		b_log[W_ERR] << "Unable to create a socket: " << strerror(errno);
		throw ServerPollError();
	}

	unsigned int reuse_addr = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	if(bind(sock, (struct sockaddr *) &fsocket, sizeof fsocket) < 0 ||
	   listen(sock, 5) < 0)
	{
		b_log[W_ERR] << "Unable to listen on " << bind_addr << ":" << port
			     << ": " << strerror(errno);
		throw ServerPollError();
	}

	read_cb = new CallBack<DaemonForkServerPoll>(this, &DaemonForkServerPoll::new_client_cb);
	read_id = glib_input_add(sock, (PurpleInputCondition)PURPLE_INPUT_READ,
				       g_callback_input, read_cb);

#if 0
	try
	{
		irc = new irc::IRC(this, 0,
		              conf.GetSection("irc")->GetItem("hostname")->String(),
			      conf.GetSection("irc")->GetItem("ping")->Integer());
	}
	catch(irc::AuthError &e)
	{
		b_log[W_ERR] << "Unable to start the IRC daemon";
		throw ServerPollError();
	}
#endif
}

DaemonForkServerPoll::~DaemonForkServerPoll()
{
	if(read_id >= 0)
		g_source_remove(read_id);
	delete read_cb;

	delete irc;
}

bool DaemonForkServerPoll::new_client_cb(void*)
{
	struct sockaddr_in newcon;
	unsigned int addrlen = sizeof newcon;
	int new_socket = accept(sock, (struct sockaddr *) &newcon, &addrlen);

	pid_t client_pid = fork();

	if(client_pid > 0)
	{
		/* Parent */
		b_log[W_ERR] << "Creating new process with pid " << client_pid;
		close(new_socket);
		return true;
	}
	else
	{
		/* Child */
		close(sock);
		g_source_remove(read_id);
		delete read_cb;
		read_cb = NULL;
		read_id = -1;
		try
		{
			irc = new irc::IRC(this, new_socket,
				      conf.GetSection("irc")->GetItem("hostname")->String(),
				      conf.GetSection("irc")->GetItem("ping")->Integer());
		}
		catch(irc::AuthError &e)
		{
			b_log[W_ERR] << "Unable to start the IRC daemon";
			getApplication()->quit();
		}
	}
	return true;
}

void DaemonForkServerPoll::log(size_t level, string msg) const
{
	if(msg.find("\n") != string::npos)
		msg = msg.substr(0, msg.find("\n"));

	string cmd = MSG_NOTICE;
	if(level & W_DEBUG)
		cmd = MSG_PRIVMSG;

	if(irc)
		irc->getUser()->send(irc::Message(cmd).setSender(irc)
							     .setReceiver(irc->getUser())
							     .addArg(msg));
	else
		std::cout << msg << std::endl;
}

void DaemonForkServerPoll::kill(irc::IRC* irc)
{
	assert(irc == this->irc);

	stop_cb = new CallBack<DaemonForkServerPoll>(this, &DaemonForkServerPoll::stopServer_cb);
	g_timeout_add(0, g_callback, stop_cb);
}

bool DaemonForkServerPoll::stopServer_cb(void*)
{
	delete stop_cb;
	stop_cb = NULL;

	delete irc;
	irc = NULL;

	getApplication()->quit();
	return false;
}
