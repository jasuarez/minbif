/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009 Marc Dequènes, Romain Bignon
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
#include "gayattitude.h"

PurplePlugin *_gayattitude_plugin = NULL;

GayAttitudeAccount* gayattitude_account_new(PurpleAccount *account)
{
	GayAttitudeAccount* gaa;

	gaa = g_new0(GayAttitudeAccount, 1);
	gaa->account = account;
	gaa->pc = purple_account_get_connection(account);
	gaa->http_handler = http_handler_new(account, gaa);
	gaa->ref_ids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	account->gc->proto_data = gaa;
	return gaa;
}

void gayattitude_account_free(GayAttitudeAccount* gaa)
{
	if (gaa->new_messages_check_timer) {
		purple_timeout_remove(gaa->new_messages_check_timer);
	}

	http_handler_free(gaa->http_handler);
	g_hash_table_destroy(gaa->ref_ids);
	g_free(gaa);
}

static const char *gayattitude_blist_icon(PurpleAccount *a, PurpleBuddy *b)
{
	return "gayattitude";
}

struct message_t
{
	gchar* message;
	gchar* from;
	time_t timestamp;
	gint64 id;

	struct message_t* next;
};

static void gayattitude_parse_contact_list(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj, xpathObj2;
	GayAttitudeAccount* gaa = handler->data;
	gchar* group_name = userdata;

	doc = htmlReadMemory(response, len, "gayattitude.xml", NULL, 0);
	if (doc == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XML Parsing).\n");
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath context init).\n");
		xmlFreeDoc(doc);
		return;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression((xmlChar*) "//div[@id='ANNUAIRE']/div[@id='RESULTATS']/div", xpathCtx);
	if(xpathObj == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath evaluation).\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return;
	}
	if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
	{
		/* Print results */
		xmlNodeSetPtr nodes = xpathObj->nodesetval;
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "number of nodes found: %i\n", nodes->nodeNr);

		PurpleGroup *group = NULL;
		if (group_name)
		{
			group = purple_find_group(group_name);
			if (!group)
			{
				group = purple_group_new(group_name);
				purple_blist_add_group(group, NULL);
			}
			g_free(group_name);
		}

		int i;
		gchar *prop = NULL;
		xmlNode *contact_node;
		for(i = 0; i < nodes->nodeNr; i++)
		{
			contact_node = nodes->nodeTab[i];
			prop = (gchar*) xmlGetProp(contact_node, (xmlChar*) "class");

			/* look for contacts */
			if (prop && g_str_has_prefix(prop, "ITEM"))
			{
				/* enforce current search node and look for contact details */
				gchar *contact_name;
				PurpleBuddy *buddy;
				xpathCtx->node = contact_node;
				xpathObj2 = xmlXPathEvalExpression((xmlChar*) "./div[@class='ITEM2']/div[@class='pseudo']/a/text()", xpathCtx);
				if (xpathObj2 == NULL)
				{
					purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath evaluation).\n");
					xmlXPathFreeContext(xpathCtx);
					xmlFreeDoc(doc);
					g_free(prop);
					return;
				}
				if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
				{
					contact_name = (gchar*) xpathObj2->nodesetval->nodeTab[0]->content;
					purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "found buddy from server: %s\n", contact_name);

					buddy = purple_find_buddy(gaa->account, contact_name);
					if (!buddy)
					{
						buddy = purple_buddy_new(gaa->account, contact_name, NULL);
						purple_blist_add_buddy(buddy, NULL, group, NULL);
						purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "added missing buddy: %s\n", contact_name);
					}
					if (strstr(prop, "ITEMONLINE"))
						purple_prpl_got_user_status(gaa->account, contact_name, "available", NULL);
					else
						purple_prpl_got_user_status(gaa->account, contact_name, "offline", NULL);
				}

				xmlXPathFreeObject(xpathObj2);
			}
			g_free(prop);
		}
	}

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
}

static void gayattitude_update_buddy_list(GayAttitudeAccount* gaa)
{
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-checklist",
			NULL, gayattitude_parse_contact_list, g_strdup("Checklist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-friendlist",
			NULL, gayattitude_parse_contact_list, g_strdup("Friendlist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-hotlist",
			NULL, gayattitude_parse_contact_list, g_strdup("Hotlist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-blogolist",
			NULL, gayattitude_parse_contact_list, g_strdup("Blogolist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-blacklist",
			NULL, gayattitude_parse_contact_list, g_strdup("Blacklist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-whitelist",
			NULL, gayattitude_parse_contact_list, g_strdup("Whitelist"), FALSE);
}

static void gayattitude_check_new_messages_and_buddy_status(GayAttitudeAccount* gaa)
{
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-online",
			NULL, gayattitude_parse_contact_list, NULL, FALSE);
}

static void gayattitude_login_cb(HttpHandler* handler, gchar *response, gsize len,
		gpointer userdata)
{
	GayAttitudeAccount *gaa = handler->data;
	purple_connection_update_progress(gaa->pc, "Authenticating", 2, 3);

	if (!g_hash_table_lookup(gaa->http_handler->cookie_table, "cookielogin"))
	{
		purple_connection_error_reason(gaa->pc,
				PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
				"Could not log in GA. (check 'username' and 'password' settings)");
	}
	else
	{
		purple_connection_set_state(gaa->pc, PURPLE_CONNECTED);
		gayattitude_update_buddy_list(gaa);
		gaa->new_messages_check_timer = g_timeout_add_seconds(GA_CHECK_INTERVAL, (GSourceFunc)gayattitude_check_new_messages_and_buddy_status, gaa);
	}
}


static void gayattitude_login(PurpleAccount *account)
{
	PurpleConnection *gc;
	GayAttitudeAccount* gaa;
	gint flags = HTTP_METHOD_POST;
	gchar *postdata;
	const char *username = purple_account_get_username(account);
	const char *password = purple_account_get_password(account);

	gc = purple_account_get_connection(account);
	gc->flags |= PURPLE_CONNECTION_NO_NEWLINES;

	gaa = gayattitude_account_new(account);

	purple_connection_set_display_name(gc, username);

	purple_connection_set_state(gc, PURPLE_CONNECTING);
	purple_connection_update_progress(gc, "Connecting", 1, 3);

	gchar* encoded_password = http_url_encode(password, TRUE);
	postdata = g_strdup_printf("login=%s&passw=%s", username, encoded_password);

	http_post_or_get(gaa->http_handler, flags , GA_HOSTNAME, "/html/login",
			postdata, gayattitude_login_cb, NULL, FALSE);

	g_free(encoded_password);
	g_free(postdata);
}

static void gayattitude_close(PurpleConnection *gc)
{
	g_return_if_fail(gc != NULL);
	g_return_if_fail(gc->proto_data != NULL);

	gayattitude_account_free(gc->proto_data);
}

static void gayattitude_parse_contact_info(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	GayAttitudeAccount *gaa = handler->data;
	GayAttitudeBuddyInfoRequest *request = userdata;

	doc = htmlReadMemory(response, len, "gayattitude.xml", NULL, 0);
	if (doc == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XML Parsing).\n");
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath context init).\n");
		xmlFreeDoc(doc);
		return;
	}

	xmlNode *info_node;
	gchar *prop;

	/* Search internal Ref ID */
	xpathObj = xmlXPathEvalExpression((xmlChar*) "//input[@type='hidden' and @name='ref_id']", xpathCtx);
	if(xpathObj == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath evaluation).\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return;
	}
	if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
	{
		info_node = xpathObj->nodesetval->nodeTab[0];
		prop = (gchar*) xmlGetProp(info_node, (xmlChar*) "value");
		if (!g_hash_table_lookup(gaa->ref_ids, request->who))
			g_hash_table_insert(gaa->ref_ids, g_strdup(request->who), prop);
		else
			g_free(prop);
	}
	xmlXPathFreeObject(xpathObj);

	if (request->advertise)
	{
		PurpleNotifyUserInfo *user_info = purple_notify_user_info_new();
		int i;
		GString *str = NULL;

		/* Search short description */
		xpathCtx->node = doc->parent;
		xpathObj = xmlXPathEvalExpression((xmlChar*) "//div[@id='PORTRAITHEADER2']/p/text()", xpathCtx);
		if(xpathObj == NULL)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath evaluation).\n");
			xmlXPathFreeContext(xpathCtx);
			xmlFreeDoc(doc);
			return;
		}
		if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
		{
			info_node = xpathObj->nodesetval->nodeTab[0];
			purple_notify_user_info_add_pair(user_info, "Short Description", (gchar*) info_node->content);
		}
		xmlXPathFreeObject(xpathObj);

		/* Search user research */
		xpathCtx->node = doc->parent;
		xpathObj = xmlXPathEvalExpression((xmlChar*) "//div[@id='bloc_recherche']/p/text()", xpathCtx);
		if(xpathObj == NULL)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "Unable to parse response (XPath evaluation).\n");
			xmlXPathFreeContext(xpathCtx);
			xmlFreeDoc(doc);
			return;
		}
		if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
		{
			for(i = 0; i < xpathObj->nodesetval->nodeNr; i++)
			{
				info_node = xpathObj->nodesetval->nodeTab[i];
				if (i == 0)
					str = g_string_new((gchar*) info_node->content);
				else
					g_string_append_printf(str, " -- %s", info_node->content);
			}
			purple_notify_user_info_add_pair(user_info, "Research", str->str);
			g_string_free(str, TRUE);
		}
		xmlXPathFreeObject(xpathObj);

		purple_notify_userinfo(gaa->pc, request->who, user_info, NULL, NULL);
		purple_notify_user_info_destroy(user_info);
	}

	/* Cleanup */
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);

	/* Chained Callback */
	if (request->callback)
		request->callback(gaa, request->callback_data);
}

static void gayattitude_request_info(GayAttitudeAccount* gaa, const char *who, gboolean advertise, GayAttitudeRequestInfoCallbackFunc callback, gpointer callback_data)
{
	gchar *url_path;
	GayAttitudeBuddyInfoRequest *request = g_new0(GayAttitudeBuddyInfoRequest, TRUE);

	url_path = g_strdup_printf("/%s", who);
	request->who = g_strdup(who);
	request->advertise = advertise;
	request->callback = callback;
	request->callback_data = callback_data;
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME_PERSO, url_path,
			NULL, gayattitude_parse_contact_info, (gpointer) request, FALSE);
	g_free(url_path);
}

static void gayattitude_get_info(PurpleConnection *gc, const char *who)
{
	gayattitude_request_info(gc->proto_data, who, TRUE, NULL, NULL);
}

static int gayattitude_do_send_im(GayAttitudeAccount *gaa, PurpleBuddy *buddy, const char *what, PurpleMessageFlags flags)
{
	gchar *ref_id = g_hash_table_lookup(gaa->ref_ids, buddy->name);
	if (!ref_id)
	{
		purple_debug_error("gayattitude", "send_im: could not find ref_id for buddy '%s'\n", buddy->name);
		return 1;
	}

	gchar* url_path = g_strdup_printf("/html/portrait/message?p=%s&pid=%s&host=&smallheader=&popup=0", buddy->name, ref_id);
	gchar* msg = http_url_encode(what, TRUE);
	gchar* postdata = g_strdup_printf("msg=%s&sendchat=Envoyer+(Shift-Entr%%82e)&fond=&sendmail=0", msg);
	http_post_or_get(gaa->pc->proto_data, HTTP_METHOD_POST, GA_HOSTNAME, url_path,
			postdata, NULL, NULL, FALSE);

	g_free(msg);
	g_free(postdata);
	g_free(url_path);
	purple_debug_info("gayattitude", "sending message to '%s'\n", buddy->name);

	return 0;
}

void gayattitude_send_im_delayed_cb(GayAttitudeAccount *gaa, GayAttitudeDelayedMessageRequest *delayed_msg)
{
	gayattitude_do_send_im(gaa, delayed_msg->buddy, delayed_msg->what, delayed_msg->flags);
}

static int gayattitude_send_im(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags)
{
	GayAttitudeAccount* gaa = gc->proto_data;
	PurpleBuddy *buddy;

	buddy = purple_find_buddy(gaa->account, who);
	if (!buddy)
	{
		purple_debug_error("gayattitude", "send_im: buddy '%s' does not exist\n", who);
		return 1;
	}

	gchar *ref_id = g_hash_table_lookup(gaa->ref_ids, who);
	if (!ref_id)
	{
		purple_debug_error("gayattitude", "send_im: ref_id for buddy '%s' is unknown, starting lookup for delayed message\n", who);

		GayAttitudeDelayedMessageRequest *delayed_msg = g_new0(GayAttitudeDelayedMessageRequest, TRUE);
		delayed_msg->gaa = gaa;
		delayed_msg->buddy = buddy;
		delayed_msg->what = g_strdup(what);
		delayed_msg->flags = flags;

		gayattitude_request_info(gaa, who, FALSE, (GayAttitudeRequestInfoCallbackFunc) gayattitude_send_im_delayed_cb, (gpointer) delayed_msg);
	}
	else
		gayattitude_do_send_im(gaa, buddy, what, flags);

	return 0;
}

static GList *gayattitude_status_types(PurpleAccount *account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}

static void gayattitude_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc = purple_account_get_connection(account);
	//const char *status_id = purple_status_get_id(status);

	g_return_if_fail(gc != NULL);

	if (!purple_status_is_active(status))
		return;

}

static void gayattitude_keepalive(PurpleConnection *gc)
{
}

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_PASSWORD_OPTIONAL,
	NULL,					/* user_splits */
	NULL,					/* protocol_options */
	NO_BUDDY_ICONS,				/* icon_spec */
	gayattitude_blist_icon,			/* list_icon */
	NULL,					/* list_emblems */
	NULL,					/* status_text */
	NULL,					/* tooltip_text */
	gayattitude_status_types,		/* away_states */
	NULL,					/* blist_node_menu */
	NULL,					/* chat_info */
	NULL,					/* chat_info_defaults */
	gayattitude_login,			/* login */
	gayattitude_close,			/* close */
	gayattitude_send_im,			/* send_im */
	NULL,					/* set_info */
	NULL,					/* send_typing */
	gayattitude_get_info,			/* get_info */
	gayattitude_set_status,			/* set_status */
	NULL,					/* set_idle */
	NULL,					/* change_passwd */
	NULL,					/* add_buddy */
	NULL,					/* add_buddies */
	NULL,					/* remove_buddy */
	NULL,					/* remove_buddies */
	NULL,					/* add_permit */
	NULL,					/* add_deny */
	NULL,					/* rem_permit */
	NULL,					/* rem_deny */
	NULL,					/* set_permit_deny */
	NULL,					/* join_chat */
	NULL,					/* reject_chat */
	NULL,					/* get_chat_name */
	NULL,					/* chat_invite */
	NULL,					/* chat_leave */
	NULL,					/* chat_whisper */
	NULL,					/* chat_send */
	gayattitude_keepalive,			/* keepalive */
	NULL,					/* register_user */
	NULL,					/* get_cb_info */
	NULL,					/* get_cb_away */
	NULL,					/* alias_buddy */
	NULL,					/* group_buddy */
	NULL,					/* rename_group */
	NULL,					/* buddy_free */
	NULL,					/* convo_closed */
	purple_normalize_nocase,		/* normalize */
	NULL,					/* set_buddy_icon */
	NULL,					/* remove_group */
	NULL,					/* get_cb_real_name */
	NULL,					/* set_chat_topic */
	NULL,					/* find_blist_chat */
	NULL,					/* roomlist_get_list */
	NULL,					/* roomlist_cancel */
	NULL,					/* roomlist_expand_category */
	NULL,					/* can_receive_file */
	NULL,					/* send_file */
	NULL,					/* new_xfer */
	NULL,					/* offline_message */
	NULL,					/* whiteboard_prpl_ops */
	NULL,					/* send_raw */
	NULL,					/* roomlist_room_serialize */
	NULL,                   		/* unregister_user */
	NULL,                   		/* send_attention */
	NULL,                   		/* get_attention_types */
	sizeof(PurplePluginProtocolInfo),	/* struct_size */
	NULL,                    		/* get_account_text_table */
	NULL,                    		/* initiate_media */
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

	"prpl-gayattitude",                                       /**< id             */
	"GayAttitude",                                            /**< name           */
	"1.0",                                  /**< version        */
	"gayattitude Protocol Plugin",                        /**  summary        */
	"www.gayattitude.com chat",    /**  description    */
	"Marc Dequènes, Romain Bignon",                                             /**< author         */
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
	GHashTable *ui_info = purple_core_get_ui_info();
	const gchar *ui_name = g_hash_table_lookup(ui_info, "version");
	if(!ui_name)
		ui_name = GA_NAME;

	option = purple_account_option_string_new("User-agent", "user-agent", ui_name);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_gayattitude_plugin = plugin;
}

PURPLE_INIT_PLUGIN(gayattitude, _init_plugin, info);
