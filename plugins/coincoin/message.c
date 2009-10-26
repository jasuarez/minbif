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

#include "message.h"

/* From gcoincoin */
static gchar *strutf8( const gchar *pc, guint uMaxChar )
{
	gunichar     uCode ;
	guchar       b ;
	gsize        uLen ;
	GString     *pString ;
	guint        uChar ;
	const gchar *pcEnd ;

	if(( pc == NULL )||( *pc == 0 ))
	{
		return NULL ;
	}

	if( uMaxChar == 0 )
	{
		uMaxChar = G_MAXUINT ;
	}

	uLen  = strlen( pc );
	pcEnd = &pc[ uLen ];
	uChar = 0 ;

	if( g_utf8_validate( pc, (gssize) uLen, NULL ) )
	{
		const gchar *pcStart = pc ;
		while(( pc < pcEnd )&&( uChar < uMaxChar ))
		{
			pc = g_utf8_next_char(pc);
			uChar++ ;
		}
		return g_strndup( pcStart, pc - pcStart );
	}

	pString = g_string_sized_new( uLen );
	while(( pc < pcEnd )&&( uChar < uMaxChar ))
	{
		b = (guchar) *pc ;
		if( b < 128 )
		{
			/* Keep ASCII characters, but remove all control characters
			 * but CR, LF and TAB. */

			if(( b > 31 )&&( b != 127 ))
			{
				g_string_append_c( pString, b );
			}
			else
			{
				switch( b )
				{
					case '\n':
					case '\r':
					case '\t':
						break ;
					default:
						b = ' ' ;
				}
				g_string_append_c( pString, b );
			}
			pc++ ;
		}
		else
		{
			uCode = g_utf8_get_char_validated( pc, -1 );
			if(( uCode != (gunichar)-1 )&&( uCode != (gunichar)-2 ))
			{
				/* Keep a valid UTF-8 character as is */
				g_string_append_unichar( pString, uCode );
				pc = g_utf8_next_char(pc);
			}
			else
			{
				/* Consider an invalid byte as an ISO-8859-1 character code.
				 * We get rid of ASCII & ISO-8859-1 control characters. */

				if(( b > 0x1F )&&( b < 0x7F ))
				{
					/* ASCII characters, excluding control characters */
					g_string_append_c( pString, b );
				}
				else if( b > 0x9F )
				{
					/* ISO-8859-1 character, excluding control character (0x7F-0x9F) */
					g_string_append_unichar( pString, (gunichar)b );
				}
				else
				{
					g_string_append_c( pString, ' ' );
				}
				pc++ ;
			}
		}
		uChar++ ;
	}

	#ifdef DEBUG
		g_assert( g_utf8_validate( pString->str, -1, NULL ) );
	#endif

	return g_string_free( pString, FALSE );
}

xmlnode* coincoin_xmlparse(gchar* response, gsize len)
{
	gchar* utf8 = strutf8(response, len);
	xmlnode* node = xmlnode_from_str(utf8, len);
	g_free(utf8);
	return node;
}

CoinCoinMessage* coincoin_message_new(gint64 id, xmlnode* post)
{
	CoinCoinMessage* msg;
	xmlnode* message = xmlnode_get_child(post, "message");
	xmlnode* info = xmlnode_get_child(post, "info");
	xmlnode* login = xmlnode_get_child(post, "login");
	static struct tm t;
	time_t tt = time(NULL);

	if(!message || !info || !login)
		return NULL;

	/* Parse time */
	if (sscanf(xmlnode_get_attrib(post, "time"), "%4d%2d%2d%2d%2d%2d", &t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) == 6)
		tt = mktime(&t);

	/* Skip chars before message. */
	gchar* data = xmlnode_get_data(message);
	while(data && (*data == '\t' || *data == '\n' || *data == '\r'))
		++data;

	msg = g_new0(CoinCoinMessage, 1);
	if(!msg)
	{
		return NULL;
	}
	msg->message = g_strdup(data);
	msg->info = g_strdup(xmlnode_get_data(info));
	msg->from = g_strdup(xmlnode_get_data(login));
	msg->timestamp = tt;
	msg->id = id;
	return msg;
}

void coincoin_message_free(CoinCoinMessage* msg)
{
	g_free(msg->message);
	g_free(msg->info);
	g_free(msg);
}

void coincoin_parse_message(CoinCoinAccount* cca, gchar* response, gsize len, gpointer userdata)
{
	PurpleConversation* convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, "board", cca->account);
	if(!convo)
		return; // not on the board channel

	xmlnode* node = coincoin_xmlparse(response, len);
	xmlnode* post;
	GSList *last_msg = cca->messages;
	GSList *iter;
	GSList *messages = NULL;
	unsigned i;

	if(!node)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "coincoin", "Unable to parse response.\n");
		return;
	}

	for(post = xmlnode_get_child(node, "post"); post; post = xmlnode_get_next_twin(post))
	{
		CoinCoinMessage* msg;
		gint64 id = strtoul(xmlnode_get_attrib(post, "id"), NULL, 10);

		/* Check if this message has already been showed. */
		for(iter = last_msg; iter && ((CoinCoinMessage*)iter->data)->id != id; iter = iter->next)
			;
		if(iter)
			break;

		msg = coincoin_message_new(id, post);
		messages = g_slist_prepend(messages, msg);

		if(strcmp(msg->from, purple_connection_get_display_name(cca->pc)))
		{
			PurpleConvChatBuddy* cb = purple_conv_chat_cb_find(PURPLE_CONV_CHAT(convo), msg->from);
			if(!cb)
				purple_conv_chat_add_user(PURPLE_CONV_CHAT(convo), msg->from, msg->info, PURPLE_CBFLAGS_NONE, FALSE);
		}
	}

	/* Flush messages (in reversed order) */
	for(iter = messages; iter; )
	{
		CoinCoinMessage* msg = iter->data;
		serv_got_chat_in(cca->pc,
				 purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)),
				 msg->from,
				 PURPLE_MESSAGE_DELAYED,
				 msg->message,
				 msg->timestamp);
		GSList* link = iter;
		iter = iter->next;
		link->next = cca->messages;
		cca->messages = link;
	}
	/* Now purge extra-messages */
	for(i = 0, iter = last_msg; iter; ++i)
	{
		if(i < CC_LAST_MESSAGE_MAX)
			iter = iter->next;
		else if(i == CC_LAST_MESSAGE_MAX)
		{
			GSList* prev;
			prev = iter;
			iter = iter->next;
			prev->next = NULL;
		}
		else
		{
			/* This user doesn't participate to conversation
			 * anymore. So it can leave channel.
			 */
			CoinCoinMessage* cur = iter->data;
			if(strcmp(cur->from, purple_connection_get_display_name(cca->pc)) &&
			   purple_conv_chat_cb_find(PURPLE_CONV_CHAT(convo), cur->from))
			{
				GSList* it = cca->messages;
				while(it && it != iter && strcmp(((CoinCoinMessage*)it->data)->from, cur->from))
					it = it->next;

				if(it == iter || !it)
					purple_conv_chat_remove_user(PURPLE_CONV_CHAT(convo), cur->from, NULL);
			}
			coincoin_message_free(cur);
			iter->data = NULL;
			iter = g_slist_delete_link(iter, iter);
		}
	}

	xmlnode_free(node);
}
