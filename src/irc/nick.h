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

#ifndef IRC_NICK_H
#define IRC_NICK_H

#include <map>
#include <purple.h>

#include "core/entity.h"
#include "irc/message.h"

class CacaImage;

namespace im
{
	class Conversation;
}

namespace irc
{
	class Server;
	class Channel;
	class ChanUser;

	using std::map;

	/** This class represents a nick on network.
	 *
	 * This is a base class.
	 */
	class Nick : public Entity
	{
		string identname, hostname, realname;
		string away;
		Server* server;
		unsigned int flags;
		vector<ChanUser*> channels;

	public:

		static const char *nick_lc_chars;
		static const char *nick_uc_chars;
		static const size_t MAX_LENGTH = 29;
		static string nickize(const string& n);

		/** States of the user */
		enum {
			REGISTERED = 1 << 0,
			PING       = 1 << 1,
			OPER       = 1 << 2
		};

		/** Build the Nick object.
		 *
		 * @param server  up-server of this nick
		 * @param nickname  nickname of nick
		 * @param identname  ident of nick
		 * @param hostname  hostname of nick
		 * @param realname  realname
		 */
		Nick(Server* server, string nickname, string identname, string hostname, string realname="");
		virtual ~Nick();

		/** Check if the given string is a valid nickname. */
		static bool isValidNickname(const string& n);

		/** Virtual method called when sending a message to this nick. */
		virtual void send(Message m) {}

		/** User joins a channel
		 *
		 * @param chan  channel to join
		 * @param status  status on channel (see ChanUser)
		 */
		ChanUser* join(Channel* chan, int status = 0);

		/** User leaves a channel
		 *
		 * @param chan  channel to leave
		 * @param message  part message
		 */
		void part(Channel* chan, string message="");

		/** Remove an ChanUser from list.
		 *
		 * @param chanuser  ChanUser object
		 */
		void removeChanUser(ChanUser* chanuser);

		/** User quits network
		 *
		 * @param message  quit message.
		 */
		void quit(string message="");

		/** Used has been kicked by someone else on a channel.
		 *
		 * @param chan  channel
		 * @param kicker  bad guy who kicked me
		 * @param reason  reason invoked to kick me
		 */
		void kicked(Channel* chan, ChanUser* kicker, string reason);

		/** User sends a privmsg to a channel
		 *
		 * @param chan  channel target
		 * @param message  text message
		 */
		void privmsg(Channel* chan, string message);

		/** User sends a privmsg to an other user.
		 *
		 * @param to  user target
		 * @param message  text message
		 */
		void privmsg(Nick* to, string message);

		/** Send a command to this user. */
		virtual int sendCommand(const string& cmd) { return PURPLE_CMD_STATUS_WRONG_TYPE; }

		/** Mode message
		 *
		 * @param sender  user who sent message
		 * @param m  mode message
		 */
		virtual void m_mode(Nick* sender, Message m);

		/** Get all channels user is on. */
		vector<ChanUser*> getChannels() const;

		/** Check if user is on one specified channel.
		 *
		 * @param chan  channel to check
		 * @return  true if user is on channel.
		 */
		bool isOn(const Channel* chan) const;

		/** Get ChanUser object of a channel.
		 *
		 * @param chan  channel
		 * @return  ChanUser instance if user is on channel, NULL othewise
		 */
		ChanUser* getChanUser(const Channel* chan) const;

		Server* getServer() const { return server; }

		/** Get the full name representation of user.
		 *
		 * @return  a string in form "nick!ident@hostname"
		 */
		virtual string getLongName() const;

		string getNickname() const { return getName(); }
		void setNickname(string n);

		string getIdentname() const { return identname; }
		void setIdentname(string i);

		string getHostname() const { return hostname; }
		void setHostname(string h);

		virtual string getRealName() const { return realname; }
		void setRealname(string r) { realname = r; }

		virtual bool retrieveInfo() const { return false; }

		void setFlag(unsigned flag) { flags |= flag; }
		void delFlag(unsigned flag) { flags &= ~flag; }
		bool hasFlag(unsigned flag) const { return flags & flag; }

		void setAwayMessage(string a) { away = a; }
		virtual string getAwayMessage() const { return away; }
		virtual bool isAway() const { return away.empty() == false; }
		virtual bool isOnline() const { return true; }

		virtual CacaImage getIcon() const;
		virtual string getIconPath() const { return ""; }
	};

	class ConvNick : public Nick
	{
	public:
		ConvNick(Server* server, string nickname, string identname, string hostname, string realname="");

		virtual im::Conversation getConversation() const = 0;
		virtual void setConversation(const im::Conversation& c) = 0;
	};

}; /* namespace irc */

#endif /* IRC_NICK_H */
