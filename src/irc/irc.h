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

#ifndef IRC_IRC_H
#define IRC_IRC_H

#include <stdint.h>
#include <string>
#include <map>
#include <exception>

#include "message.h"
#include "server.h"
#include "im/auth.h"

class _CallBack;
class ServerPoll;

namespace im
{
	class IM;
	class Buddy;
	class Conversation;
	class FileTransfert;
};

/** IRC related classes */
namespace irc
{
	using std::string;
	using std::map;

	/** Raised when user can't authenticate himself on server */
	class AuthError : public std::exception {};

	class User;
	class Nick;
	class Channel;
	class DCC;

	/** This class represents the user's server.
	 *
	 * It provides all user's commands handlers, and several
	 * mechanismes to control the IRC network.
	 */
	class IRC : public Server
	{
		ServerPoll* poll;
		int fd;
		int read_id;
		_CallBack *read_cb;
		int ping_id;
		time_t ping_freq;
		time_t uptime;
		_CallBack *ping_cb;
		User* user;
		im::IM* im;
		im::Auth *im_auth;
		map<string, Nick*> users;
		map<string, Channel*> channels;
		map<string, Server*> servers;
		vector<DCC*> dccs;
		vector<string> motd;

		struct command_t
		{
			const char* cmd;
			void (IRC::*func)(Message);
			size_t minargs;
			unsigned count;
			unsigned flags;
		};
		static command_t commands[];

		void cleanUpNicks();
		void cleanUpChannels();
		void cleanUpServers();
		void cleanUpDCC();


		/** Callback when it receives a new incoming message from socket. */
		bool readIO(void*);

		bool check_channel_join(void*);

		void m_nick(Message m);     /**< Handler for the NICK message */
		void m_user(Message m);     /**< Handler for the USER message */
		void m_pass(Message m);     /**< Handler for the PASS message */
		void m_quit(Message m);     /**< Handler for the QUIT message */
		void m_ping(Message m);     /**< Handler for the PING message */
		void m_pong(Message m);     /**< Handler for the PONG message */
		void m_who(Message m);      /**< Handler for the WHO message */
		void m_whois(Message m);    /**< Handler for the WHOIS message */
		void m_whowas(Message m);   /**< Handler for the WHOWAS message */
		void m_version(Message m);  /**< Handler for the VERSION message */
		void m_privmsg(Message m);  /**< Handler for the PRIVMSG message */
		void m_stats(Message m);    /**< Handler for the STATS message */
		void m_connect(Message m);  /**< Handler for the CONNECT message */
		void m_squit(Message m);    /**< Handler for the SQUIT message */
		void m_map(Message m);      /**< Handler for the MAP message */
		void m_admin(Message m);    /**< Handler for the ADMIN message */
		void m_join(Message m);     /**< Handler for the JOIN message */
		void m_part(Message m);     /**< Handler for the PART message */
		void m_list(Message m);     /**< Handler for the LIST message */
		void m_mode(Message m);     /**< Handler for the MODE message */
		void m_names(Message m);    /**< Handler for the NAMES message */
		void m_topic(Message m);    /**< Handler for the TOPIC message */
		void m_ison(Message m);     /**< Handler for the ISON message */
		void m_invite(Message m);   /**< Handler for the INVITE message */
		void m_kick(Message m);     /**< Handler for the KICK message */
		void m_kill(Message m);     /**< Handler for the KILL message */
		void m_svsnick(Message m);  /**< Handler for the SVSNICK message */
		void m_away(Message m);     /**< Handler for the AWAY message */
		void m_motd(Message m);     /**< Handler for the MOTD message */
		void m_oper(Message m);     /**< Handler for the OPER message */
		void m_wallops(Message m);  /**< Handler for the WALLOPS message */
		void m_rehash(Message m);   /**< Handler for the REHASH message */
		void m_die(Message m);      /**< Handler for the DIE message */
		void m_cmd(Message m);      /**< Handler for the CMD message */

	public:

		/** Create an instance of the IRC class
		 *
		 * @param poll  the server poll used by minbif
		 * @param fd  file descriptor where read and write to user
		 * @param hostname  server's hostname
		 * @param ping_freq  frequence of pings
		 */
		IRC(ServerPoll* poll, int fd, string hostname, unsigned ping_freq);
		~IRC();

		User* getUser() const { return user; }

		im::IM* getIM() const { return im; }
		im::Auth* getIMAuth() const { return im_auth; }

		/** Ends the auth sequence.
		 *
		 * It checks if user has sent all requested parameters to
		 * authenticate himself, and checks for password.
		 *
		 * If authentification success, it create the im::IM instance,
		 * sends all welcome replies, create account servers, etc.
		 */
		void sendWelcome();

		/** User quits.
		 *
		 * @param reason  text used in the QUIT message
		 */
		void quit(string reason = "");

		void addChannel(Channel* chan);
		Channel* getChannel(string channame) const;
		void removeChannel(string channame);

		void rehash(bool verbose = true);
		void setMotd(const string& path);

		void addNick(Nick* nick);
		Nick* getNick(string nick, bool case_sensitive = false) const;
		Nick* getNick(const im::Buddy& buddy) const;
		Nick* getNick(const im::Conversation& c) const;
		void removeNick(string nick);
		void renameNick(Nick* n, string newnick);

		void addServer(Server* server);
		Server* getServer(string server) const;
		void removeServer(string server);

		DCC* createDCCSend(const im::FileTransfert& ft, Nick* from);
		DCC* createDCCGet(Nick* from, string filename, uint32_t addr,
				  uint16_t port, ssize_t size, _CallBack* callback);
		void updateDCC(const im::FileTransfert& ft, bool destroy = false);

		/** Callback used by glibc to check user ping */
		bool ping(void*);

		/** Send a notice to a user.
		 *
		 * @param user  destination
		 * @param message  text message sent
		 */
		void notice(Nick* user, string message);

		/** Send a privmsg to a user.
		 *
		 * @param user  destination
		 * @param message text message sent.
		 */
		void privmsg(Nick* user, string message);
	};

}; /* namespace irc */

#endif /* IRC_IRC_H */
