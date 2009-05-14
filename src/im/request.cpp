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

#include "request.h"
#include "im.h"
#include "purple.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "../util.h"

namespace im {

class RequestNick : public irc::Nick
{
public:

	RequestNick(irc::Server* server, string nickname, string identname, string hostname, string realname="")
		: irc::Nick(server, nickname, identname, hostname, realname)
	{}

	void send(irc::Message m)
	{
		if(m.getCommand() != MSG_PRIVMSG || m.getReceiver() != this)
			return;

		string answer = m.getArg(0);
		irc::IRC* irc = Purple::getIM()->getIRC();
		Request* request = Request::getFirstRequest();
		if(!request)
		{
			privmsg(irc->getUser(), "No active question");
			return;
		}

		try
		{
			request->process(answer);
		}
		catch(RequestFieldNotFound &e)
		{
			privmsg(irc->getUser(), "ERROR: Answer '" + answer + "' is not valid.");
			request->display();
			return;
		}

		Request::closeFirstRequest();
	}
};

void RequestFieldAction::runCallback()
{
	callback(data, id);
}

Request::Request(PurpleRequestType _type, const string& _title, const string& _question)
	: title(_title),
	  question(_question),
	  type(_type)
{

}

RequestFieldList::~RequestFieldList()
{
	map<string, RequestField*>::iterator it;
	for(it = fields.begin(); it != fields.end(); ++it)
		delete it->second;
}

void RequestFieldList::addField(RequestField* field)
{
	fields[field->getLabel()] = field;
}

RequestField* RequestFieldList::getField(const string& label) const
{
	map<string, RequestField*>::const_iterator it = fields.find(label);
	if(it == fields.end())
		throw RequestFieldNotFound();

	return it->second;
}

void RequestFieldList::process(const string& answer) const
{
	RequestField* field = getField(answer);
	field->runCallback();
}

void RequestFieldList::display() const
{
	irc::IRC* irc = Purple::getIM()->getIRC();
	nick->privmsg(irc->getUser(), "New request: " + title);
	nick->privmsg(irc->getUser(), question);
	for(map<string, RequestField*>::const_iterator it = fields.begin();
	    it != fields.end();
	    ++it)
	{
		nick->privmsg(irc->getUser(), "- " + it->first + ": " + it->second->getText());
	}
}

void RequestInput::process(const string& answer) const
{
	callback(user_data, answer.c_str());
}

void RequestInput::display() const
{
	irc::IRC* irc = Purple::getIM()->getIRC();
	nick->privmsg(irc->getUser(), "New request: " + title);
	nick->privmsg(irc->getUser(), question);
	nick->privmsg(irc->getUser(), "Default value is: " + default_value);
	nick->privmsg(irc->getUser(), "Type answer:");
}

void Request::close()
{
	purple_request_close(type, this);
}

/* STATIC */

PurpleRequestUiOps Request::uiops =
{
	Request::request_input,
        Request::request_choice,
        Request::request_action,
        NULL,//finch_request_fields,
        NULL,//finch_request_file,
        NULL,//finch_close_request,
        NULL,//finch_request_folder,
        NULL,
        NULL,
        NULL,
        NULL
};

vector<Request*> Request::requests;
RequestNick* Request::nick = NULL;

void Request::init()
{
	purple_request_set_ui_ops(&uiops);

	irc::IRC* irc = Purple::getIM()->getIRC();
	nick = new RequestNick(irc, "request", "request", irc->getServerName());
	irc->addNick(nick);
}

Request* Request::getFirstRequest()
{
	return requests.empty() ? NULL : requests.front();
}

void Request::closeFirstRequest()
{
	Request* request = getFirstRequest();
	if(!request)
		return;

	request->close();
	delete request;
	requests.erase(requests.begin());

	if(!requests.empty())
		requests.front()->display();
}

void* Request::request_input(const char *title, const char *primary,
			const char *secondary, const char *default_value,
			gboolean multiline, gboolean masked, gchar *hint,
			const char *ok_text, GCallback ok_cb,
			const char *cancel_text, GCallback cancel_cb,
			PurpleAccount *account, const char *who, PurpleConversation *conv,
			void *user_data)
{
	RequestInput* request = new RequestInput(PURPLE_REQUEST_INPUT, title, primary, default_value, (PurpleRequestInputCb)ok_cb, user_data);
	requests.push_back(request);
	if(requests.size() == 1)
		request->display();

	return requests.back();
}

void* Request::request_action(const char *title, const char *primary,
			const char *secondary, int default_value,
			PurpleAccount *account, const char *who, PurpleConversation *conv,
			void *user_data, size_t actioncount,
			va_list actions)
{
	RequestFieldList* request = new RequestFieldList(PURPLE_REQUEST_ACTION, title, primary);
	for(size_t i = 0; i < actioncount; ++i)
	{
		const char *text = va_arg(actions, const char *);
		string tmp = text;
		PurpleRequestActionCb callback = va_arg(actions, PurpleRequestActionCb);

		request->addField(new RequestFieldAction((int)i, strlower(stringtok(tmp, " ")), text, callback, user_data));
	}
	requests.push_back(request);
	if(requests.size() == 1)
		request->display();

	return requests.back();
}

void* Request::request_choice(const char *title, const char *primary,
			const char *secondary, int default_value,
			const char *ok_text, GCallback ok_cb,
			const char *cancel_text, GCallback cancel_cb,
			PurpleAccount *account, const char *who, PurpleConversation *conv,
			void *user_data, va_list choices)
{
	RequestFieldList* request = new RequestFieldList(PURPLE_REQUEST_CHOICE, title, primary);
	const char* text;

	while ((text = va_arg(choices, const char *)))
	{
		int val = va_arg(choices, int);
		string tmp;

		request->addField(new RequestFieldAction(val, strlower(stringtok(tmp, " ")), text, (PurpleRequestChoiceCb)ok_cb, user_data));
	}
	request->addField(new RequestFieldAction(0, "cancel", "Cancel", (PurpleRequestChoiceCb)cancel_cb, user_data));

	requests.push_back(request);
	if(requests.size() == 1)
		request->display();

	return requests.back();
}

}; /* namespace im */
