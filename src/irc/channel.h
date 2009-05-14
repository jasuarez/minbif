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

#ifndef IRC_CHANNEL_H
#define IRC_CHANNEL_H

#include <string>
#include <vector>

#include "message.h"
#include "../entity.h"
#include "../util.h"

namespace irc
{
	using std::vector;
	using std::string;


	class Nick;
	class IRC;
	class Channel;

	/** This class represents a channel user. */
	class ChanUser : public Entity
	{
		Nick* nick;
		Channel* chan;
		int status;

	public:

		/** Channel user modes */
		enum mode_t {
			VOICE   = PURPLE_CBFLAGS_VOICE,
			HALFOP  = PURPLE_CBFLAGS_HALFOP,
			OP      = PURPLE_CBFLAGS_OP,
			FOUNDER = PURPLE_CBFLAGS_FOUNDER,
			TYPING  = PURPLE_CBFLAGS_TYPING,
		};

		/** Structure used in arraw to convert a mode flag to char. */
		static struct m2c_t
		{
			mode_t mode;
			char c;
		}  m2c[];

		/** Build the ChanUser object.
		 *
		 * @param chan  channel of this user
		 * @param nick  nick of this user
		 * @param status  status of user on channel
		 */
		ChanUser(Channel* chan, Nick* nick, int status = 0);

		string getName() const;
		string getLongName() const;

		bool hasStatus(int flag) const { return status & flag; }
		void setStatus(int flag) { status |= flag; }
		void delStatus(int flag) { status &= ~flag; }

		Nick* getNick() const { return nick; }

		Channel* getChannel() const { return chan; }

		/** Get the mode flag from char */
		static mode_t c2mode(char c);

		/** Get the mode char from flag */
		static char mode2c(mode_t m);

		/** Create Message object of modes
		 *
		 * @param add  is it added or removed modes?
		 * @param modes  modes used. If = 0, use user modes
		 * @return  generated message
		 */
		Message getModeMessage(bool add, int modes = 0) const;
	};

	/** This class represents an IRC Channel. */
	class Channel : public Entity
	{
	protected:
		IRC* irc;

	private:
		vector<ChanUser*> users;
		string topic;

	public:

		/** Build the Channel object.
		 *
		 * @param irc  the IRC object of main server
		 * @param name  channel name.
		 */
		Channel(IRC* irc, string name);
		virtual ~Channel();

		/** Check the validity of a channel name
		 *
		 * @param name  name to check
		 * @return  true if channel name is correct
		 */
		static bool isChanName(const string& name)
		{
			return (!name.empty() && name.find(' ') == string::npos &&
				(isStatusChannel(name) || isRemoteChannel(name)));
		}

		static bool isStatusChannel(const string& name) { return (!name.empty() && name[0] == '&'); }
		static bool isRemoteChannel(const string& name) { return (!name.empty() && name[0] == '#'); }

		virtual bool isStatusChannel() const { return (!getName().empty() && getName()[0] == '&'); }
		virtual bool isRemoteChannel() const { return (!getName().empty() && getName()[0] == '#'); }

		/** Add a nick on channel.
		 *
		 * @param nick  nick to add on channel
		 * @param status  initial status of user.
		 * @return  the ChanUser instance
		 */
		ChanUser* addUser(Nick* nick, int status=0);

		/** Remove an user from channel.
		 *
		 * @param nick  user to remove
		 * @param message  optionnal message to send to all *other* channel users.
		 */
		virtual void delUser(Nick* nick, Message message = Message());

		/** Count users on channel. */
		size_t countUsers() const { return users.size(); }

		/** Get a vector of channel users. */
		vector<ChanUser*> getChanUsers() const { return users; }

		/** Get a channel user. */
		virtual ChanUser* getChanUser(string nick) const;

		/** Get topic */
		virtual string getTopic() const { return topic; }

		void setTopic(Entity* who, string topic);

		/** Mode message
		 *
		 * @param sender  user who sent message
		 * @param m  mode message
		 */
		void m_mode(Nick* sender, Message m);

		/** Show banlist to user */
		virtual void showBanList(Nick* to) = 0;

		/** Add a ban
		 *
		 * @param from  user who adds ban
		 * @param nick  nick in mask
		 * @param ident  ident in mask
		 * @param host  hostname in mask
		 * @param accid  account id in mask
		 */
		virtual void processAddBan(Nick* from, string nick, string ident, string host, string accid) = 0;

		/** Remove a ban
		 *
		 * @param from  user who removes ban
		 * @param nick  nick in mask
		 * @param ident  ident in mask
		 * @param host  hostname in mask
		 * @param accid  account id in mask
		 */
		virtual void processRemoveBan(Nick* from, string nick, string ident, string host, string accid) = 0;

		/** Set a mode on a channel user.
		 *
		 * @param sender  entity which sets mode.
		 * @param modes  flags set on channel.
		 * @param chanuser  channel user impacted.
		 */
		void setMode(const Entity* sender, int modes, ChanUser* chanuser);


		/** Remove a mode on a channel user.
		 *
		 * @param sender  entity which removes mode.
		 * @param modes  flags set on channel.
		 * @param chanuser  channel user impacted.
		 */
		void delMode(const Entity* sender, int modes, ChanUser* chanuser);

		/** Broadcast a message to all channel users.
		 *
		 * @param m  message sent to all channel users
		 * @param butone  optionnal user which will not receive message.
		 */
		virtual void broadcast(Message m, Nick* butone = NULL);
	};
}; /*namespace irc*/

#endif /* IRC_CHANNEL_H */
