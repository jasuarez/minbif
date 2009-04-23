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

#include "request.h"

namespace im {

Request::Request(string _title, string _question, PurpleRequestChoiceCb _callback)
	: title(_title),
	  question(_question),
	  callback(_callback)
{

}

void Request::addField(RequestField field)
{
	fields[field.getLabel()] = field;
}

RequestField Request::getField(const string& label) const
{
	map<string, RequestField>::const_iterator it = fields.find(label);
	if(it == fields.end())
		throw RequestFieldNotFound();

	return it->second;
}

/* STATIC */

PurpleRequestUiOps Request::uiops =
{
        NULL,//finch_request_input,
        NULL,//finch_request_choice,
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

void Request::init()
{
	purple_request_set_ui_ops(&uiops);
}


void* Request::request_action(const char *title, const char *primary,
			const char *secondary, int default_value,
			PurpleAccount *account, const char *who, PurpleConversation *conv,
			void *user_data, size_t actioncount,
			va_list actions)
{
	return NULL;
}

}; /* namespace im */
