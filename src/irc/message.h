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

#include "replies.h"

class Entity;

namespace irc
{
	using std::string;
	using std::vector;

	class MalformedMessage : public std::exception {};

	class _StoredEntity
	{
		const Entity* entity;
		string name;

	public:
		_StoredEntity() : entity(NULL) {}

		void setEntity(const Entity* e) { entity = e; name.clear(); }
		void setName(string n) { name = n; entity = NULL; }

		bool isSet() const { return entity || !name.empty(); }
		const Entity* getEntity() const { return entity; }
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
		Message& setSender(const Entity* entity);
		Message& setSender(string name);
		Message& setReceiver(const Entity* entity);
		Message& setReceiver(string name);
		Message& addArg(string);
		Message& setArg(size_t, string);

		string getCommand() const { return cmd; }
		const Entity* getSender() const { return sender.getEntity(); }
		const Entity* getReceiver() const { return receiver.getEntity(); }
		string getArg(size_t n) const;
		size_t countArgs() const { return args.size(); }
		vector<string> getArgs() const { return args; }

		string format() const;
		void rebuildWithQuotes();
		static Message parse(string s);
	};
}; /* namespace irc */
#endif /* IRC_MESSAGE_H */
