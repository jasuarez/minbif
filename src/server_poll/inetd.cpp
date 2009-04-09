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

#include <cassert>
#include <glib/gmain.h>

#include "inetd.h"
#include "../irc/irc.h"
#include "../irc/user.h"
#include "../config.h"
#include "../callback.h"
#include "../log.h"
#include "../bitlbee.h"

InetdServerPoll::InetdServerPoll(Bitlbee* application)
	: ServerPoll(application),
	  irc(NULL),
	  stop_cb(NULL)
{
	try
	{
		irc = new IRC(this, 0,
		              conf.GetSection("irc")->GetItem("hostname")->String(),
			      conf.GetSection("irc")->GetItem("command_chan")->String(),
			      conf.GetSection("irc")->GetItem("ping")->Integer());
	}
	catch(IRCAuthError &e)
	{
		b_log[W_ERR] << "Unable to start the IRC daemon";
		throw ServerPollError();
	}
}

InetdServerPoll::~InetdServerPoll()
{
	delete irc;
}

void InetdServerPoll::log(string msg) const
{
	irc->getUser()->send(Message(MSG_NOTICE).setSender(irc)
						.setReceiver(irc->getUser())
						.addArg(msg));
}

void InetdServerPoll::kill(IRC* irc)
{
	assert(irc == this->irc);

	stop_cb = new CallBack<InetdServerPoll>(this, &InetdServerPoll::stopServer_cb);
	g_timeout_add(0, g_callback, stop_cb);
}

bool InetdServerPoll::stopServer_cb(void*)
{
	delete stop_cb;
	stop_cb = NULL;

	delete irc;
	irc = NULL;

	getApplication()->quit();
	return false;
}
