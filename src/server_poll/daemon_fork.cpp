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
#include <sys/stat.h>
#include <arpa/inet.h>

#include "daemon_fork.h"
#include "core/config.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "irc/message.h"
#include "irc/replies.h"
#include "core/config.h"
#include "core/callback.h"
#include "core/log.h"
#include "core/minbif.h"
#include "core/util.h"
#include "sockwrap/sock.h"
#include "sockwrap/sockwrap.h"

DaemonForkServerPoll::DaemonForkServerPoll(Minbif* application)
	: ServerPoll(application),
	  irc(NULL),
	  sock(-1),
	  read_cb(NULL)
{
	ConfigSection* section = conf.GetSection("irc")->GetSection("daemon");
	if(section->Found() == false)
	{
		b_log[W_ERR] << "Missing section irc/daemon";
		throw ServerPollError();
	}

	if(section->GetItem("background")->Boolean())
	{
		int r = fork();
		if(r < 0)
		{
			b_log[W_ERR] << "Unable to start in background: " << strerror(errno);
			throw ServerPollError();
		}
		else if(r > 0)
			exit(EXIT_SUCCESS); /* parent exits. */

		setsid();

		for (r=getdtablesize();r>=0;--r) close(r);
		r=open("/dev/null",O_RDWR); /* open stdin */
		dup(r); /* stdout */
		dup(r); /* stderr */

		umask(027);
		string path = conf.GetSection("path")->GetItem("users")->String();
		chdir(path.c_str());
	}

	struct addrinfo *addrinfo_bind, *res, hints;
	string bind_addr = section->GetItem("bind")->String();
	uint16_t port = (uint16_t)section->GetItem("port")->Integer();
	unsigned int reuse_addr = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if(getaddrinfo(bind_addr.c_str(), t2s(port).c_str(), &hints, &addrinfo_bind))
	{
		b_log[W_ERR] << "Could not parse address " << bind_addr << ":" << port;
		throw ServerPollError();
	}

	for(res = addrinfo_bind; res && sock < 0; res = res->ai_next)
	{
		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(sock < 0)
			continue;

		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);
		if(bind(sock, res->ai_addr, res->ai_addrlen) < 0 ||
		   listen(sock, 5) < 0)
		{
			close(sock);
			sock = -1;
		}
	}

	if(sock < 0)
		b_log[W_ERR] << "Unable to listen on " << bind_addr << ":" << port
			     << ": " << strerror(errno);
	else
	{

		read_cb = new CallBack<DaemonForkServerPoll>(this, &DaemonForkServerPoll::new_client_cb);
		read_id = glib_input_add(sock, (PurpleInputCondition)PURPLE_INPUT_READ,
					       g_callback_input, read_cb);
	}

	freeaddrinfo(addrinfo_bind);
	if(!read_cb)
		throw ServerPollError();
}

DaemonForkServerPoll::~DaemonForkServerPoll()
{
	if(read_id >= 0)
		g_source_remove(read_id);
	delete read_cb;
	if(sock >= 0)
		close(sock);

	delete irc;

	for(vector<child_t*>::iterator it = childs.begin(); it != childs.end(); ++it)
	{
		close((*it)->fd);
		delete (*it)->read_cb;
		g_source_remove((*it)->read_id);
		delete *it;
	}
}

bool DaemonForkServerPoll::new_client_cb(void*)
{
	struct sockaddr_in newcon;
	socklen_t addrlen = sizeof newcon;
	int new_socket = accept(sock, (struct sockaddr *) &newcon, &addrlen);

	if(new_socket < 0)
	{
		b_log[W_WARNING] << "Could not accept new connection: " << strerror(errno);
		return true;
	}

	int fds[2];
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1)
	{
		b_log[W_WARNING] << "Unable to create IPC socket for client: " << strerror(errno);
		fds[0] = fds[1] = -1;
	}
	sock_make_nonblocking(fds[0]);
	sock_make_nonblocking(fds[1]);

	pid_t client_pid = fork();

	if(client_pid < 0)
	{
		b_log[W_ERR] << "Unable to fork while receiving a new connection: " << strerror(errno);
		return true;
	}
	else if(client_pid > 0)
	{
		/* Parent */
		b_log[W_INFO] << "Creating new process with pid " << client_pid;
		close(new_socket);
		if(fds[0] >= 0)
		{
			child_t* child = new child_t();
			child->fd = fds[0];
			child->read_cb = new CallBack<DaemonForkServerPoll>(this, &DaemonForkServerPoll::ipc_read, child);
			child->read_id = glib_input_add(child->fd, (PurpleInputCondition)PURPLE_INPUT_READ,
						       g_callback_input, child->read_cb);
			childs.push_back(child);
			close(fds[1]);
		}
	}
	else
	{
		/* Child */
		close(sock);
		sock = -1;
		g_source_remove(read_id);
		read_id = -1;
		delete read_cb;
		read_cb = NULL;

		if(fds[1] >= 0)
		{
			sock = fds[1];
			read_cb = new CallBack<DaemonForkServerPoll>(this, &DaemonForkServerPoll::ipc_read);
			read_id = glib_input_add(sock, (PurpleInputCondition)PURPLE_INPUT_READ,
						       g_callback_input, read_cb);
			close(fds[0]);

			/* Cleanup all childs accumulated when I was parent. */
			for(vector<child_t*>::iterator it = childs.begin(); it != childs.end(); it = childs.erase(it))
			{
				child_t* child = *it;
				close(child->fd);
				delete child->read_cb;
				g_source_remove(child->read_id);
				delete child;
			}
		}

		try
		{
			irc = new irc::IRC(this, SockWrapper::Builder(new_socket, new_socket),
				      conf.GetSection("irc")->GetItem("hostname")->String(),
				      conf.GetSection("irc")->GetItem("ping")->Integer());
		}
		catch(IRCError &e)
		{
			b_log[W_ERR] << "Unable to start the IRC daemon";
			getApplication()->quit();
		}
	}
	return true;
}

DaemonForkServerPoll::ipc_cmds_t DaemonForkServerPoll::ipc_cmds[] = {
	{ MSG_WALLOPS,    &DaemonForkServerPoll::m_wallops,  2 },
	{ MSG_REHASH,     &DaemonForkServerPoll::m_rehash,   0 },
	{ MSG_DIE,        &DaemonForkServerPoll::m_die,      2 },
	{ MSG_OPER,       &DaemonForkServerPoll::m_oper,     1 },
};

void DaemonForkServerPoll::m_oper(child_t* child, irc::Message m)
{
	if(child)
		ipc_master_broadcast(m, child);
	b_log[W_SNO|W_INFO] << m.getArg(0) << " is now an IRC Operator!";
}

void DaemonForkServerPoll::m_wallops(child_t* child, irc::Message m)
{
	if(child)
		ipc_master_broadcast(m);
	else if(irc)
		irc->getUser()->send(irc::Message(MSG_WALLOPS).setSender(m.getArg(0))
				                         .addArg(m.getArg(1)));
}

void DaemonForkServerPoll::m_rehash(child_t* child, irc::Message m)
{
	rehash();
}

void DaemonForkServerPoll::m_die(child_t* child, irc::Message m)
{
	if(child)
		ipc_master_broadcast(m);

	b_log[W_SNO|W_INFO] << "Shutdown requested by " << m.getArg(0) << ": " << m.getArg(1);
	if(irc)
		irc->quit("Shutdown requested by " + m.getArg(0));
	else
		getApplication()->quit();
}

bool DaemonForkServerPoll::ipc_read(void* data)
{
	child_t* child = NULL;
	if(data)
		child = static_cast<child_t*>(data);

	static char buf[512];
	ssize_t r;
	int fd;
	if(child)
		fd = child->fd;
	else
		fd = sock;
	if((r = recv(fd, buf, sizeof buf - 1, MSG_PEEK)) <= 0)
	{
		if(r < 0 && sockerr_again())
			return true;
		if(child)
		{
			b_log[W_INFO] << "IPC: a child left: " << strerror(errno);
			for(vector<child_t*>::iterator it = childs.begin(); it != childs.end();)
				if(child->fd == (*it)->fd)
					it = childs.erase(it);
				else
					++it;

			g_source_remove(child->read_id);
			delete child->read_cb;
			delete child;
		}
		else
		{
			b_log[W_INFO|W_SNO] << "IPC: master left: " << strerror(errno);
			g_source_remove(read_id);
			read_id = -1;
			delete read_cb;
			read_cb = NULL;
			sock = -1;
		}
		return false;
	}

	buf[r] = 0;
	char* eol;
	if(!(eol = strstr(buf, "\r\n")))
		return true;
	else
		r = eol - buf + 2;

	if(recv(fd, buf, r, 0) != r)
		return false;
	buf[r - 2] = 0;

	irc::Message m = irc::Message::parse(buf);

	unsigned i = 0;
	for(; i < (sizeof ipc_cmds / sizeof *ipc_cmds) && m.getCommand() != ipc_cmds[i].cmd; ++i)
		;

	if(i >= (sizeof ipc_cmds / sizeof *ipc_cmds))
	{
		b_log[W_WARNING] << "Received unknown command from IPC: " << buf;
		return true;
	}

	if(m.countArgs() < ipc_cmds[i].min_args)
	{
		b_log[W_WARNING] << "Received malformated command from IPC: " << buf;
		return true;
	}

	(this->*ipc_cmds[i].func)(child, m);

	return true;
}

bool DaemonForkServerPoll::ipc_master_send(child_t* child, const irc::Message& m)
{
	if(!child)
		return false;

	string msg = m.format();
	if(write(child->fd, msg.c_str(), msg.size()) <= 0)
	{
		b_log[W_ERR] << "Error while sending: " << strerror(errno);
		return false;
	}
	return true;
}

bool DaemonForkServerPoll::ipc_master_broadcast(const irc::Message& m, child_t* butone)
{
	bool ret = false;
	for(vector<child_t*>::iterator it = childs.begin(); it != childs.end(); ++it)
		if(butone != *it && ipc_master_send(*it, m))
			ret = true;
	return ret;
}

bool DaemonForkServerPoll::ipc_child_send(const irc::Message& m)
{
	if(sock < 0)
		return false;

	string msg = m.format();
	if(write(sock, msg.c_str(), msg.size()) <= 0)
	{
		b_log[W_ERR] << "Error while sending: " << strerror(errno);
		return false;
	}
	return true;
}

bool DaemonForkServerPoll::ipc_send(const irc::Message& m)
{
	if(irc)
		return ipc_child_send(m);
	else
		return ipc_master_broadcast(m);
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
	else if(!(level & W_SNO))
		std::cout << msg << std::endl;
}

void DaemonForkServerPoll::rehash()
{
	if(irc)
		irc->rehash();
	else
		ipc_master_broadcast(irc::Message(MSG_REHASH));
}

void DaemonForkServerPoll::kill(irc::IRC* irc)
{
	assert(!irc || irc == this->irc);

	_CallBack* stop_cb = new CallBack<DaemonForkServerPoll>(this, &DaemonForkServerPoll::stopServer_cb);
	g_timeout_add(0, g_callback_delete, stop_cb);
}

bool DaemonForkServerPoll::stopServer_cb(void*)
{
	delete irc;
	irc = NULL;

	getApplication()->quit();
	return false;
}
