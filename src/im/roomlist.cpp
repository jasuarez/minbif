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

#include "im/roomlist.h"
#include "im/conversation.h"
#include "im/purple.h"
#include "im/im.h"
#include "irc/irc.h"
#include "irc/user.h"
#include "core/log.h"
#include "core/util.h"

namespace im {

/* METHODS */

RoomList::RoomList(PurpleRoomlist* rl)
	: rlist(rl)
{

}

RoomList::~RoomList()
{

}

void RoomList::create(PurpleRoomlist* list)
{
}

void RoomList::set_fields(PurpleRoomlist* list, GList* fields)
{
	GList* l;
	string str;
	for(l = fields; l; l = l->next)
	{
		PurpleRoomlistField *f = (PurpleRoomlistField*)l->data;
		if(!f->hidden)
			str += string(f->label) + " ";
	}
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc->getUser()->send(irc::Message(RPL_LISTSTART).setSender(irc)
					                .setReceiver(irc->getUser())
					                .addArg("Channel")
					                .addArg(str));
}

void RoomList::add_room(PurpleRoomlist* list, PurpleRoomlistRoom* room)
{
	PurpleConnection *gc = purple_account_get_connection(list->account);
	if(!gc)
		return;

	char* name;
	string str;
	PurplePluginProtocolInfo* prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_connection_get_prpl(gc));

	if(prpl_info != NULL && prpl_info->roomlist_room_serialize)
		name = prpl_info->roomlist_room_serialize(room);
	else
		name = g_strdup(purple_roomlist_room_get_name(room));

	GList *iter, *field;
	for (iter = purple_roomlist_room_get_fields(room),
	     field = purple_roomlist_get_fields(list);
	     iter && field;
	     iter = iter->next, field = field->next)
	{
		PurpleRoomlistField *f = (PurpleRoomlistField*)field->data;

		if (purple_roomlist_field_get_hidden(f))
			continue;

		switch (purple_roomlist_field_get_type(f)) {
			case PURPLE_ROOMLIST_FIELD_BOOL:
				str += iter->data ? "True " : "False ";
				break;
			case PURPLE_ROOMLIST_FIELD_INT:
				str += t2s((size_t)iter->data) + " ";
				break;
			case PURPLE_ROOMLIST_FIELD_STRING:
				str += string((const char*)iter->data) + " ";
				break;
		}
	}

	irc::IRC* irc = Purple::getIM()->getIRC();
	irc->getUser()->send(irc::Message(RPL_LIST).setSender(irc)
				                   .setReceiver(irc->getUser())
				                   .addArg(Conversation::normalizeIRCName(name, Account(list->account)))
				                   .addArg(str));

	g_free(name);
}

void RoomList::in_progress(PurpleRoomlist* list, gboolean flag)
{
	if(!flag)
	{
		irc::IRC* irc = Purple::getIM()->getIRC();
		irc->getUser()->send(irc::Message(RPL_LISTEND).setSender(irc)
							      .setReceiver(irc->getUser())
							      .addArg("End of /LIST"));
	}
}

void RoomList::destroy(PurpleRoomlist* list)
{
}

/* STATICS */

PurpleRoomlistUiOps RoomList::ui_ops =
{
	NULL, /* void (*show_with_account)(PurpleAccount *account); **< Force the ui to pop up a dialog and get the list */
	&RoomList::create, /* void (*create)(PurpleRoomlist *list); **< A new list was created. */
	&RoomList::set_fields, /* void (*set_fields)(PurpleRoomlist *list, GList *fields); **< Sets the columns. */
	&RoomList::add_room, /* void (*add_room)(PurpleRoomlist *list, PurpleRoomlistRoom *room); **< Add a room to the list. */
	&RoomList::in_progress, /* void (*in_progress)(PurpleRoomlist *list, gboolean flag); **< Are we fetching stuff still? */
	&RoomList::destroy, /* void (*destroy)(PurpleRoomlist *list); **< We're destroying list. */

	NULL, /* void (*_purple_reserved1)(void); */
	NULL, /* void (*_purple_reserved2)(void); */
	NULL, /* void (*_purple_reserved3)(void); */
	NULL  /* void (*_purple_reserved4)(void); */
};

void RoomList::init()
{
	purple_roomlist_set_ui_ops(&ui_ops);
}

void RoomList::uninit()
{
	purple_roomlist_set_ui_ops(NULL);
}

} /* ns im */
