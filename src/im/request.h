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

#ifndef IM_REQUEST_H
#define IM_REQUEST_H

#include <map>
#include <string>
#include <exception>
#include <purple.h>

#include "account.h"

namespace im
{
	using std::map;
	using std::string;

	class RequestFieldNotFound : public std::exception {};

	class RequestField
	{
		int id;
		string label;
		string text;

	public:

		RequestField() : id(-1) {}

		RequestField(int _id, string _label, string _text)
			: id(_id),
			  label(_label),
			  text(_text)
		{}

		int getID() const { return id; }
		string getLabel() const { return label; }
		string getText() const { return text; }
	};

	class Request
	{
		map<string, RequestField> fields;
		string title;
		string question;
		PurpleRequestChoiceCb callback;
		Account account;

		static PurpleRequestUiOps uiops;
		static void* request_action(const char *title, const char *primary,
				        const char *secondary, int default_value,
					PurpleAccount *account, const char *who, PurpleConversation *conv,
					void *user_data, size_t actioncount,
					va_list actions);


	public:

		static void init();

		Request(string title, string question, PurpleRequestChoiceCb callback);

		void addField(RequestField field);
		RequestField getField(const string& label) const;
	};

}; /* namespace im */

#endif /* IM_REQUEST_H */
