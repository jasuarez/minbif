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

#ifndef IM_PROTOCOL_H
#define IM_PROTOCOL_H

#include <libpurple/purple.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace im
{
	class Protocol
	{
		PurplePlugin* plugin;

	public:

		class Option
		{
			PurplePrefType type;
			string name;
			string value;
			string text;

			string nameFromText(string s) const;

		public:

			Option(PurplePrefType type, string name, string text);
			Option();

			bool operator==(string s) const;

			PurplePrefType getType() const { return type; }
			string getName() const { return name; }
			string getText() const { return text; }

			void setValue(string v) { value = v; }
			string getValue() const { return value; }
			int getValueInt() const;
			bool getValueBool() const;
		};

		Protocol(PurplePlugin* plugin);
		Protocol();

		bool operator==(const Protocol& proto) const;
		bool operator!=(const Protocol& proto) const;

		bool isValid() const { return plugin != NULL; }
		string getName() const;
		string getID() const;
		string getPurpleID() const;

		vector<Option> getOptions() const;

		PurplePluginProtocolInfo* getPurpleProtocol() const;
	};

}; /* namespace im */

#endif /* IM_PROTOCOL_H */
