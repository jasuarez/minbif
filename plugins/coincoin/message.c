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

#include <string.h>

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
	gchar *data, *ptr;
	static struct tm t;
	time_t tt = time(NULL);

	if(!message || !info || !login)
		return NULL;

	/* Parse time */
	if (sscanf(xmlnode_get_attrib(post, "time"), "%4d%2d%2d%2d%2d%2d", &t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) == 6)
	{
		t.tm_year -= 1900;
		t.tm_mon -= 1;
		tt = mktime(&t);
	}

	/* Skip chars before message. */
	ptr = data = xmlnode_get_data(message);
	while(ptr && (*ptr == '\t' || *ptr == '\n' || *ptr == '\r'))
		++ptr;

	msg = g_new0(CoinCoinMessage, 1);
	if(!msg)
	{
		return NULL;
	}
	msg->message = g_strdup(ptr);
	msg->info = xmlnode_get_data(info);
	msg->from = xmlnode_get_data(login);
	msg->timestamp = tt;
	msg->id = id;
	msg->ref = 1;
	msg->multiple = FALSE;

	g_free(data);
	return msg;
}

void coincoin_message_free(CoinCoinMessage* msg)
{
	g_free(msg->message);
	g_free(msg->info);
	g_free(msg->from);
	g_free(msg);
}

gchar* coincoin_convert_message(CoinCoinAccount* cca, const char* msg)
{
	GString* s;
	const gchar *start, *next;

	if(purple_account_get_bool(cca->account, "no_reformat_messages", FALSE))
		return g_strdup(msg);

	s = g_string_sized_new(strlen(msg));

	for(start = msg; *start; start = next)
	{
		next = g_utf8_next_char(start);
		while(*next && *next != ' ')
			next = g_utf8_next_char(next);

		if(next > start+2 && *next && next[-1] == ':')
		{
			unsigned ref = 1;
			const gchar *end = start;
			gchar *nickname;

			while(*end && *end != ':' && *end != '\xc2')
				end = g_utf8_next_char(end);

			nickname = g_strndup(start, end-start);
			if (*end == ':')
				++end;
			if(*end >= '0' && *end <= '9')
			{
				ref = strtoul(end, NULL, 10);
			}
			else if(*end == '\xc2')
			{
				if(end[1] == '\xb9') ref = 1;       // ¹
				else if(end[1] == '\xb2') ref = 2;  // ²
				else if(end[1] == '\xb3') ref = 3;  // ³
			}

			purple_debug(PURPLE_DEBUG_ERROR, "coincoin", "Nickname: [%s], ref: [%d].\n", nickname, ref);

			GSList *m;
			CoinCoinMessage* cur = NULL;
			unsigned found = 0;
			for(m = cca->messages; m; m = m->next)
			{
				cur = m->data;
				if (!strcasecmp(cur->from, nickname) && ++found == ref)
					break;
			}
			g_free(nickname);
			if(m)
			{
				struct tm t;
				localtime_r(&cur->timestamp, &t);
				g_string_append_printf(s, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
				if (cur->multiple)
					g_string_append_printf(s, ":%d", cur->ref);
				continue;
			}
		}
		if(*next == ' ')
			next = g_utf8_next_char(next);

		g_string_append_len(s, start, next-start);
	}
	return g_string_free(s, FALSE);
}

static void coincoin_message_ref(CoinCoinMessage* msg, GSList* messages)
{
	GString* s = g_string_sized_new(strlen(msg->message));
	gchar *start, *next;
	struct tm t;

	localtime_r(&msg->timestamp, &t);
	for(start = msg->message; *start; start = next)
	{
		next = g_utf8_next_char(start);
		/* totoz */
		if(*start == '[' && *(start+1) == ':')
		{
			gchar* end = start;
			while(*end && *end != ']')
				end = g_utf8_next_char(end);
			if(*end == ']')
			{
				++end;
				g_string_append(s, "<FONT COLOR=\"dark green\">");
				g_string_append_len(s, start, end-start);
				g_string_append(s, "</FONT>");
				next = end;
			}
			else
				g_string_append_len(s, start, next-start);
		}
		/* msg refs */
		else if((*start >= '0' && *start <= '9') || *start == ':')
		{
			unsigned ref = 1;
			gboolean ref_no_secs = FALSE;
			gchar* end = start;
			gchar* clock;

			while(*end && ((*end >= '0' && *end <= '9') || *end == ':'))
				end = g_utf8_next_char(end);

			/* Detect ¹²³ unicode refs. */
			if(*end == '\xc2')
			{
				if(end[1] == '\xb9') ref = 1;       // ¹
				else if(end[1] == '\xb2') ref = 2;  // ²
				else if(end[1] == '\xb3') ref = 3;  // ³
			}

			clock = g_strndup(start, end-start);
			if(sscanf(clock, "%02d:%02d:%02d", &t.tm_hour, &t.tm_min, &t.tm_sec) == 3 ||
			   (sscanf(clock, "%02d:%02d", &t.tm_hour, &t.tm_min) == 2 && (ref_no_secs = TRUE)))
			{
				GSList* m;
				struct tm m_t;
				for(m = messages; m; m = m->next)
				{
					CoinCoinMessage* cur = m->data;
					localtime_r(&cur->timestamp, &m_t);
					if (t.tm_hour == m_t.tm_hour &&
					    t.tm_min == m_t.tm_min &&
					    (ref_no_secs || (t.tm_sec == m_t.tm_sec && cur->ref == ref)))
						break;
				}
				if(m)
				{
					g_string_append(s, ((CoinCoinMessage*)m->data)->from);
					g_string_append(s, ": ");
				}
				g_string_append(s, "<FONT COLOR=\"blue\">");
				g_string_append(s, clock);
				g_string_append(s, "</FONT>");
			}
			else
				g_string_append(s, clock);

			g_free(clock);

			next = end;
		}
		else
			g_string_append_len(s, start, next-start);
	}
	g_free(msg->message);
	msg->message = g_string_free(s, FALSE);
}

void coincoin_parse_message(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	CoinCoinAccount* cca = handler->data;
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
		if(!msg)
			continue;
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
		if(!purple_account_get_bool(cca->account, "no_reformat_messages", FALSE))
			coincoin_message_ref(msg, cca->messages);

		serv_got_chat_in(cca->pc,
				 purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)),
				 msg->from,
				 PURPLE_MESSAGE_DELAYED,
				 msg->message,
				 msg->timestamp);
		if(cca->messages && ((CoinCoinMessage*)cca->messages->data)->timestamp == msg->timestamp)
		{
			msg->multiple = ((CoinCoinMessage*)cca->messages->data)->multiple = TRUE;
			msg->ref = ((CoinCoinMessage*)cca->messages->data)->ref + 1;
		}

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
