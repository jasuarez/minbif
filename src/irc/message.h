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

#ifndef IRC_MESSAGE_H
#define IRC_MESSAGE_H

#include <string>
#include <vector>
#include <exception>

#define RPL_WELCOME     "001"
#define RPL_YOURHOST    "002"
#define RPL_CREATED     "003"
#define RPL_MYINFO      "004"
#define RPL_ISUPPORT    "005"
#define RPL_WHOISUSER   "311"
#define RPL_WHOISSERVER "312"
#define RPL_ENDOFWHOIS  "318"
#define RPL_VERSION     "351"
#define RPL_NAMREPLY    "353"
#define RPL_ENDOFNAMES  "366"
#define RPL_ENDOFWHOWAS "369"
#define RPL_MOTD        "372"
#define RPL_MOTDSTART   "375"
#define RPL_ENDOFMOTD   "376"

#define ERR_NOSUCHNICK       "401"
#define ERR_NOSUCHCHANNEL    "403"
#define ERR_WASNOSUCHNICK    "406"
#define ERR_UNKNOWNCOMMAND   "421"
#define ERR_NONICKNAMEGIVEN  "431"
#define ERR_ERRONEUSNICKNAME "432"
#define ERR_NICKNAMEINUSE    "433"
#define ERR_NICKTOOFAST      "438"
#define ERR_NOTREGISTERED    "451"
#define ERR_NEEDMOREPARAMS   "461"
#define ERR_ALREADYREGISTRED "462"
#define ERR_CHANFORWARDING   "470" /* :lem.freenode.net 470 uei #c ##c :Forwarding to another channel */

#define MSG_PRIVMSG     "PRIVMSG"
#define MSG_NOTICE      "NOTICE"
#define MSG_MODE        "MODE"
#define MSG_JOIN        "JOIN"
#define MSG_PART        "PART"
#define MSG_QUIT        "QUIT"
#define MSG_ERROR       "ERROR"
#define MSG_NICK        "NICK"
#define MSG_PING        "PING"
#define MSG_PONG        "PONG"
#define MSG_USER        "USER"
#define MSG_PASS        "PASS"
#define MSG_VERSION     "VERSION"
#define MSG_WHOIS       "WHOIS"
#define MSG_WHOWAS      "WHOWAS"

class Entity;

using std::string;
using std::vector;

class MalformedMessage : public std::exception {};

class _StoredEntity
{
	Entity* entity;
	string name;

public:
	_StoredEntity() : entity(NULL) {}

	void setEntity(Entity* e) { entity = e; name.clear(); }
	void setName(string n) { name = n; entity = NULL; }

	bool isSet() const { return entity || !name.empty(); }
	Entity* getEntity() const { return entity; }
	string getName() const;
	string getLongName() const;
};

class Message
{
	string cmd;
	_StoredEntity sender;
	_StoredEntity receiver;
	vector<string> args;
public:

	Message(string command);
	Message() {}
	~Message();

	Message& setCommand(string command);
	Message& setSender(Entity* entity);
	Message& setSender(string name);
	Message& setReceiver(Entity* entity);
	Message& setReceiver(string name);
	Message& addArg(string);
	Message& setArg(size_t, string);

	string getCommand() const { return cmd; }
	Entity* getSender() const { return sender.getEntity(); }
	Entity* getReceiver() const { return receiver.getEntity(); }
	string getArg(size_t n) const;
	size_t countArgs() const { return args.size(); }
	vector<string> getArgs() const { return args; }

	string format() const;
	static Message parse(string s);
};

#endif /* IRC_MESSAGE_H */
