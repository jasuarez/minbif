/*
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

#include "core/callback.h"
#include "irc/nick.h"
#include "irc/conv_entity.h"

namespace irc {

ConvEntity::ConvEntity(im::Conversation _conv)
	: conv(_conv)
{

}

void ConvEntity::enqueueMessage(const string& text, int delay)
{
	if (!delay)
	{
		conv.sendMessage(text);
		return;
	}
	enqueued_messages.push_back(text);

	if (enqueued_messages.size() == 1)
		g_timeout_add(delay, g_callback_delete, new CallBack<ConvEntity>(this, &ConvEntity::flush_messages, NULL));
}

bool ConvEntity::flush_messages(void*)
{
	string buf;
	for (vector<string>::iterator s = enqueued_messages.begin();
	     s != enqueued_messages.end();
	     s = enqueued_messages.erase(s))
	{
		if((*s)[0] == '\001')
		{
			if (!buf.empty())
			{
				conv.sendMessage(buf);
				buf.clear();
			}
			conv.sendMessage(*s);
		}
		else
		{
			if (!buf.empty())
				buf += '\n';
			buf += *s;
		}
	}
	if (!buf.empty())
		conv.sendMessage(buf);
	return false;
}

ConvNick::ConvNick(Server* server, im::Conversation conv, string nickname,
		   string identname, string hostname, string realname)
	: Nick(server, nickname, identname, hostname, realname),
	  ConvEntity(conv)
{}

void ConvNick::sendMessage(Nick* to, const string& t, bool action)
{
	string line = t;
	if(action)
		line = "\001ACTION " + line + "\001";
	to->send(irc::Message(MSG_PRIVMSG).setSender(this)
					  .setReceiver(to)
					  .addArg(line));
}

} /* ns irc */
