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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>
#include <exception>

#define RPL_WELCOME     "001"
#define RPL_YOURHOST    "002"
#define RPL_CREATED     "003"
#define RPL_MYINFO      "004"
#define RPL_ISUPPORT    "005"
#define RPL_MOTDSTART   "375"
#define RPL_MOTD        "372"
#define RPL_ENDOFMOTD   "376"
#define MSG_PRIVMSG     "PRIVMSG"
#define MSG_NOTICE      "NOTICE"
#define MSG_MODE        "MODE"
#define MSG_JOIN        "JOIN"
#define MSG_PART        "PART"
#define MSG_QUIT        "QUIT"

class Nick;
class IRC;

using std::string;
using std::vector;

class MalformedMessage : public std::exception {};

class Message
{
	string cmd;
	string sender;
	string receiver;
	vector<string> args;
public:

	Message(string command);
	~Message();

	Message& setSender(const Nick* nick);
	Message& setSender(const IRC* me);
	Message& setReceiver(const Nick* nick);
	Message& setReceiver(string r);
	Message& addArg(string);

	string format() const;
};

#endif /* MESSAGE_H */
