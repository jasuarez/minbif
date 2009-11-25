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

#include "irc/replies.h"

class Entity;

namespace irc
{
	using std::string;
	using std::vector;

	class MalformedMessage : public std::exception {};

	class Message
	{
	public:
		class StoredArg
		{
			string str;
			bool is_long;
		public:
			StoredArg(const string& _str, bool _is_long)
				: str(_str), is_long(_is_long || _str.find(' ') != string::npos)
			{}
			const string& getStr() const { return str; }
			void setStr(const string& _str) { str = _str; is_long = str.find(' ') != string::npos; }
			bool isLong() const { return is_long; }
		};
		typedef vector<StoredArg> ArgVector;
	private:
		class StoredEntity
		{
			const Entity* entity;
			string name;

		public:
			StoredEntity() : entity(NULL) {}

			void setEntity(const Entity* e) { entity = e; name.clear(); }
			void setName(string n) { name = n; entity = NULL; }

			bool isSet() const { return entity || !name.empty(); }
			const Entity* getEntity() const { return entity; }
			string getName() const;
			string getLongName() const;
		};

		string cmd;
		StoredEntity sender;
		StoredEntity receiver;
		ArgVector args;
	public:

		Message(string command);
		Message() {}
		~Message();

		Message& setCommand(string command);
		Message& setSender(const Entity* entity);
		Message& setSender(string name);
		Message& setReceiver(const Entity* entity);
		Message& setReceiver(string name);
		Message& addArg(string str, bool is_long = false);
		Message& setArg(size_t pos, string str, bool is_long = false);

		string getCommand() const { return cmd; }
		const Entity* getSender() const { return sender.getEntity(); }
		const Entity* getReceiver() const { return receiver.getEntity(); }
		string getArg(size_t n) const;
		size_t countArgs() const { return args.size(); }
		ArgVector getArgs() const { return args; }
		vector<string> getArgsStr() const;

		string format() const;
		void rebuildWithQuotes();
		static Message parse(string s);
	};
}; /* namespace irc */
#endif /* IRC_MESSAGE_H */
