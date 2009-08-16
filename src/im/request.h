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

#ifndef IM_REQUEST_H
#define IM_REQUEST_H

#include <map>
#include <vector>
#include <string>
#include <exception>
#include <libpurple/purple.h>

#include "account.h"

namespace im
{
	using std::map;
	using std::vector;
	using std::string;

	class RequestFieldNotFound : public std::exception {};
	class RequestNick;

	class RequestField
	{
	public:

		virtual int getID() const = 0;
		virtual void setLabel(const string& label) = 0;
		virtual string getLabel() const = 0;
		virtual string getText() const = 0;
		virtual void runCallback() = 0;
	};

	class RequestFieldAction : public RequestField
	{
		int id;
		string label;
		string text;
		PurpleRequestChoiceCb callback;
		void* data;

	public:

		RequestFieldAction() : id(-1) {}

		RequestFieldAction(int _id, string _label, string _text, PurpleRequestChoiceCb _callback, void* _data)
			: id(_id),
			  label(_label),
			  text(_text),
			  callback(_callback),
			  data(_data)
		{}

		int getID() const { return id; }
		void setLabel(const string& l) { label = l; }
		string getLabel() const { return label; }
		string getText() const { return text; }
		void runCallback();
	};

	class Request
	{
	protected:
		string title;
		string question;
		PurpleRequestType type;

		static PurpleNotifyUiOps notify_ops;
		static void* notify_message(PurpleNotifyMsgType type, const char *title,
				                const char *primary, const char *secondary);

		static PurpleRequestUiOps uiops;
		static RequestNick* nick;
		static void displayRequest(const Request& request);
		static void * request_input(const char *title, const char *primary,
			const char *secondary, const char *default_value,
			gboolean multiline, gboolean masked, gchar *hint,
			const char *ok_text, GCallback ok_cb,
			const char *cancel_text, GCallback cancel_cb,
			PurpleAccount *account, const char *who, PurpleConversation *conv,
			void *user_data);
		static void* request_action(const char *title, const char *primary,
				        const char *secondary, int default_value,
					PurpleAccount *account, const char *who, PurpleConversation *conv,
					void *user_data, size_t actioncount,
					va_list actions);
		static void * request_choice(const char *title, const char *primary,
					const char *secondary, int default_value,
					const char *ok_text, GCallback ok_cb,
					const char *cancel_text, GCallback cancel_cb,
					PurpleAccount *account, const char *who, PurpleConversation *conv,
					void *user_data, va_list choices);
		static void* request_fields(const char *title, const char *primary,
				const char *secondary, PurpleRequestFields *allfields,
				const char *ok, GCallback ok_cb,
				const char *cancel, GCallback cancel_cb,
				PurpleAccount *account, const char *who, PurpleConversation *conv,
				void *userdata);
		static void* request_file(const char *title, const char *filename,
				          gboolean savedialog,
					  GCallback ok_cb, GCallback cancel_cb,
					  PurpleAccount *account, const char *who, PurpleConversation *conv,
					  void *user_data);

	public:

		static vector<Request*> requests;
		static void init();
		static void uninit();
		static Request* getFirstRequest();
		static void closeFirstRequest();

		Request(PurpleRequestType type, const string& title, const string& question);
		virtual ~Request() {}

		virtual void process(const string& answer) const = 0;
		virtual void display() const = 0;
		virtual void close();
	};

	class RequestInput : public Request
	{
		string default_value;
		PurpleRequestInputCb callback;
		void* user_data;

	public:

		RequestInput(PurpleRequestType type, const string& title, const string& question,
		             const string& _default_value, PurpleRequestInputCb _callback, void* _user_data)
			: Request(type, title, question),
			  default_value(_default_value),
			  callback(_callback),
			  user_data(_user_data)
		{}

		virtual void process(const string& answer) const;
		virtual void display() const;
	};

	class RequestFieldList : public Request
	{
		map<string, RequestField*> fields;

	public:

		RequestFieldList(PurpleRequestType type, const string& title, const string& question)
			: Request(type, title, question)
		{}
		~RequestFieldList();

		void addField(RequestField* field);
		RequestField* getField(const string& label) const;

		virtual void process(const string& answer) const;
		virtual void display() const;
	};

}; /* namespace im */

#endif /* IM_REQUEST_H */
