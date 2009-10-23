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

#include "connection.h"
#include "coincoin.h"

PurplePlugin *_coincoin_plugin = NULL;

CoinCoinAccount* coincoin_account_new(PurpleAccount *account)
{
	CoinCoinAccount* cca;
	unsigned i;

	cca = g_new0(CoinCoinAccount, 1);
	cca->account = account;
	cca->pc = purple_account_get_connection(account);
	cca->cookie_table = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, g_free);
	cca->hostname_ip_cache = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, g_free);

	g_hash_table_replace(cca->cookie_table, g_strdup("test_cookie"),
			g_strdup("1"));

	for (i = 0; i < CC_LAST_MESSAGE_MAX; i++)
		cca->last_messages[i] = 0;
	cca->hostname = NULL;

	account->gc->proto_data = cca;
	return cca;
}

void coincoin_account_free(CoinCoinAccount* cca)
{

	if (cca->new_messages_check_timer) {
		purple_timeout_remove(cca->new_messages_check_timer);
	}

	purple_debug_info("coincoin", "destroying %d incomplete connections\n",
			g_slist_length(cca->conns));

	while (cca->conns != NULL)
		http_connection_destroy(cca->conns->data);

	while (cca->dns_queries != NULL) {
		PurpleDnsQueryData *dns_query = cca->dns_queries->data;
		purple_debug_info("coincoin", "canceling dns query for %s\n",
					purple_dnsquery_get_host(dns_query));
		cca->dns_queries = g_slist_remove(cca->dns_queries, dns_query);
		purple_dnsquery_destroy(dns_query);
	}

	g_hash_table_destroy(cca->cookie_table);
	g_hash_table_destroy(cca->hostname_ip_cache);
	g_free(cca->hostname);
	g_free(cca);
}

static const char *coincoin_blist_icon(PurpleAccount *a, PurpleBuddy *b)
{
	return "coincoin";
}

struct message_t
{
	gchar* message;
	gchar* from;
	time_t timestamp;
	gint64 id;

	struct message_t* next;
};

static void coincoin_parse_message(CoinCoinAccount* cca, gchar* response, gsize len, gpointer userdata)
{
	xmlnode* node = xmlnode_from_str(response, len);
	xmlnode* post;
	gint64 last_messages[CC_LAST_MESSAGE_MAX] = {0};
	unsigned last_i = 0;
	unsigned i;
	struct message_t* messages = NULL;
	PurpleConversation* convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, "board", cca->account);

	if(!convo)
		return; // not on the board channel

	if(!node)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "coincoin", "Unable to parse response.\n");
		return;
	}

	for(post = xmlnode_get_child(node, "post"); post; post = xmlnode_get_next_twin(post))
	{
		xmlnode* message = xmlnode_get_child(post, "message");
		xmlnode* login = xmlnode_get_child(post, "login");
		gint64 id = strtoul(xmlnode_get_attrib(post, "id"), NULL, 10);
		struct message_t* msg;
		struct tm t;
		time_t tt = time(NULL);

		for(i = 0; i < CC_LAST_MESSAGE_MAX && cca->last_messages[i] != id; ++i)
			;
		if(i < CC_LAST_MESSAGE_MAX)
			break;

		if(last_i < CC_LAST_MESSAGE_MAX)
			last_messages[last_i++] = id;

		/* Parse time */
		if (sscanf(xmlnode_get_attrib(post, "time"), "%4d%2d%2d%2d%2d%2d", &t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) == 6)
			tt = mktime(&t);

		/* Skip chars before message. */
		gchar* data = xmlnode_get_data(message);
		while(data && (*data == '\t' || *data == '\n' || *data == '\r'))
			++data;

		msg = malloc(sizeof(struct message_t));
		msg->message = g_strdup(data);
		msg->from = g_strdup(xmlnode_get_data(login));
		msg->timestamp = tt;
		msg->id = id;
		msg->next = messages;
		messages = msg;
	}

	/* Flush messages (in reversed order) */
	while(messages)
	{
		struct message_t* msg = messages;
		serv_got_chat_in(cca->pc,
				 purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)),
				 msg->from,
				 (cca->last_messages[0] == 0) ? PURPLE_MESSAGE_DELAYED : 0,
				 msg->message,
				 msg->timestamp);
		g_free(msg->message);
		g_free(msg->from);
		messages = msg->next;
		free(msg);
	}

	/* Update last_messages ids */
	for(i = 0; last_i < CC_LAST_MESSAGE_MAX; ++i, ++last_i)
		last_messages[last_i] = cca->last_messages[i];
	for(i = 0; i < CC_LAST_MESSAGE_MAX; ++i)
		cca->last_messages[i] = last_messages[i];

	xmlnode_free(node);
}

static void coincoin_check_new_messages(CoinCoinAccount* cca)
{
	gint flags = HTTP_METHOD_GET;
	if (purple_account_get_bool(cca->account, "ssl", FALSE))
		flags |= HTTP_METHOD_SSL;

	http_post_or_get(cca, flags , cca->hostname,
			purple_account_get_string(cca->account, "board", CC_DEFAULT_BOARD),
			NULL, coincoin_parse_message, NULL, FALSE);
}

static void coincoin_login_cb(CoinCoinAccount *cca, gchar *response, gsize len,
		gpointer userdata)
{
	purple_connection_update_progress(cca->pc, "Authenticating", 2, 3);
	xmlnode* node = xmlnode_from_str(response, len);
	if(!node || strcmp(node->name, "board"))
	{
		purple_connection_error_reason(cca->pc,
				PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
				"This is not a DaCode board. (check 'board' and 'post' settings)");
	}
	else
	{
		purple_connection_set_state(cca->pc, PURPLE_CONNECTED);
		serv_got_joined_chat(cca->pc, 1, "board");

		coincoin_parse_message(cca, response, len, userdata);
		cca->new_messages_check_timer = g_timeout_add_seconds(CC_CHECK_INTERVAL, (GSourceFunc)coincoin_check_new_messages, cca);
	}
	if(node)
		xmlnode_free(node);
}


static void coincoin_login(PurpleAccount *account)
{
	PurpleConnection *gc;
	CoinCoinAccount* cca;
	gint flags = HTTP_METHOD_GET;
	char **parts, **part;
	const char *username = purple_account_get_username(account);

	gc = purple_account_get_connection(account);
	gc->flags |= PURPLE_CONNECTION_NO_NEWLINES;

	cca = coincoin_account_new(account);

	parts = g_strsplit(username, "@", 2);
	purple_connection_set_display_name(gc, parts[0]);
	cca->hostname = g_strdup(parts[1]);
	g_strfreev(parts);

	/* Error localized in libpurple jabber.c */
	if (purple_account_get_bool(account, "ssl", FALSE))
	{
		if(!purple_ssl_is_supported())
		{
			purple_connection_error_reason (purple_account_get_connection(account),
					PURPLE_CONNECTION_ERROR_NO_SSL_SUPPORT,
					"Server requires TLS/SSL for login.  No TLS/SSL support found.");
			return;
		}
		else
			flags |= HTTP_METHOD_SSL;
	}

	purple_connection_set_state(gc, PURPLE_CONNECTING);
	purple_connection_update_progress(gc, "Connecting", 1, 3);

	g_hash_table_replace(cca->cookie_table, g_strdup("login"), g_strdup(purple_connection_get_display_name(gc)));

	parts = g_strsplit(purple_connection_get_password(gc), ";", -1);
	for(part = parts; *part; ++part)
	{
		char** keys = g_strsplit(*part, "=", 2);
		g_hash_table_replace(cca->cookie_table, g_strdup(keys[0]), g_strdup(keys[1]));
		g_strfreev(keys);
	}
	g_strfreev(parts);
	http_post_or_get(cca, flags , cca->hostname,
			purple_account_get_string(account, "board", CC_DEFAULT_BOARD),
			NULL, coincoin_login_cb, NULL, FALSE);
}

static void coincoin_close(PurpleConnection *gc)
{
	g_return_if_fail(gc != NULL);
	g_return_if_fail(gc->proto_data != NULL);

	coincoin_account_free(gc->proto_data);
}

static void coincoin_get_info(PurpleConnection *gc, const char *who)
{

}

static void coincoin_chat_leave (PurpleConnection *gc, int id)
{

}

static void coincoin_message_posted(CoinCoinAccount *cca, gchar *response, gsize len,
		gpointer userdata)
{
	if(len)
		purple_debug(PURPLE_DEBUG_ERROR, "coincoin", "Unable to send message to tribune.\n");
}

static int coincoin_chat_send(PurpleConnection *gc, int id, const char *what, PurpleMessageFlags flags)
{
	CoinCoinAccount* cca = gc->proto_data;
	gint http_flags = HTTP_METHOD_POST;

	if (purple_account_get_bool(cca->account, "ssl", FALSE))
		http_flags |= HTTP_METHOD_SSL;

	gchar* msg = http_url_encode(what, 1);
	gchar* postdata = g_strdup_printf("message=%s&section=1", msg);
	http_post_or_get(cca, http_flags , cca->hostname,
			purple_account_get_string(cca->account, "post", CC_DEFAULT_POST),
			postdata, coincoin_message_posted, NULL, FALSE);
	g_free(postdata);
	g_free(msg);
	return 0;
}

static GList *coincoin_status_types(PurpleAccount *account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}

static void coincoin_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc = purple_account_get_connection(account);
	//const char *status_id = purple_status_get_id(status);

	g_return_if_fail(gc != NULL);

	if (!purple_status_is_active(status))
		return;

}

static void coincoin_keepalive(PurpleConnection *gc)
{
	purple_debug_info("coincoin", "keepalive\n");
}

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_PASSWORD_OPTIONAL,
	NULL,					/* user_splits */
	NULL,					/* protocol_options */
	NO_BUDDY_ICONS,		/* icon_spec */
	coincoin_blist_icon,		/* list_icon */
	NULL,			/* list_emblems */
	NULL,					/* status_text */
	NULL,					/* tooltip_text */
	coincoin_status_types,	/* away_states */
	NULL,					/* blist_node_menu */
	NULL,	/* chat_info */
	NULL,	/* chat_info_defaults */
	coincoin_login,		/* login */
	coincoin_close,		/* close */
	NULL,		/* send_im */
	NULL,					/* set_info */
	NULL,					/* send_typing */
	coincoin_get_info,		/* get_info */
	coincoin_set_status,		/* set_status */
	NULL,					/* set_idle */
	NULL,					/* change_passwd */
	NULL,		/* add_buddy */
	NULL,					/* add_buddies */
	NULL,	/* remove_buddy */
	NULL,					/* remove_buddies */
	NULL,					/* add_permit */
	NULL,					/* add_deny */
	NULL,					/* rem_permit */
	NULL,					/* rem_deny */
	NULL,					/* set_permit_deny */
	NULL,		/* join_chat */
	NULL,					/* reject_chat */
	NULL,	/* get_chat_name */
	NULL,	/* chat_invite */
	coincoin_chat_leave,		/* chat_leave */
	NULL,					/* chat_whisper */
	coincoin_chat_send,		/* chat_send */
	coincoin_keepalive,		/* keepalive */
	NULL,					/* register_user */
	NULL,					/* get_cb_info */
	NULL,					/* get_cb_away */
	NULL,					/* alias_buddy */
	NULL,					/* group_buddy */
	NULL,					/* rename_group */
	NULL,					/* buddy_free */
	NULL,					/* convo_closed */
	purple_normalize_nocase,	/* normalize */
	NULL,					/* set_buddy_icon */
	NULL,					/* remove_group */
	NULL,					/* get_cb_real_name */
	NULL,	/* set_chat_topic */
	NULL,					/* find_blist_chat */
	NULL,	/* roomlist_get_list */
	NULL,	/* roomlist_cancel */
	NULL,					/* roomlist_expand_category */
	NULL,					/* can_receive_file */
	NULL,	/* send_file */
	NULL,	/* new_xfer */
	NULL,					/* offline_message */
	NULL,					/* whiteboard_prpl_ops */
	NULL,			/* send_raw */
	NULL,					/* roomlist_room_serialize */
	NULL,                   /* unregister_user */
	NULL,                   /* send_attention */
	NULL,                   /* get_attention_types */
	sizeof(PurplePluginProtocolInfo),    /* struct_size */
	NULL,                    /* get_account_text_table */
	NULL,                    /* initiate_media */
	NULL					 /* can_do_media */
};

static gboolean load_plugin (PurplePlugin *plugin) {

	return TRUE;
}

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                            /**< priority       */

	"prpl-coincoin",                                       /**< id             */
	"CoinCoin",                                            /**< name           */
	"1.0",                                  /**< version        */
	"coincoin Protocol Plugin",                        /**  summary        */
	"The Coincoin Protocol",    /**  description    */
	"Romain Bignon",                                             /**< author         */
	"http://symlink.me/wiki/minbif",                                     /**< homepage       */

	load_plugin,                                      /**< load           */
	NULL,                                             /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	&prpl_info,                                       /**< extra_info     */
	NULL,                                             /**< prefs_info     */
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void _init_plugin(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
	PurpleAccountUserSplit *split;

	split = purple_account_user_split_new("Server", CC_DEFAULT_HOSTNAME, '@');
	prpl_info.user_splits = g_list_append(prpl_info.user_splits, split);

	option = purple_account_option_string_new("Board path", "board", CC_DEFAULT_BOARD);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new("Post path", "post", CC_DEFAULT_POST);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new("Use SSL", "ssl", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_coincoin_plugin = plugin;
}

PURPLE_INIT_PLUGIN(coincoin, _init_plugin, info);
