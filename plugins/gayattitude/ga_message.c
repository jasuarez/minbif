
#define _GNU_SOURCE
#include "ga_message.h"
#include "ga_parsing.h"
#include <time.h>


static int ga_message_send_real(GayAttitudeAccount *gaa, GayAttitudeBuddy *gabuddy, const char *what, PurpleMessageFlags flags)
{
	gchar *url_path, *postdata, *msg;
	GayAttitudeConversationInfo *conv_info;

	purple_debug_info("gayattitude", "ga_message: about to send message to '%s'.\n", gabuddy->buddy->name);

	conv_info = g_hash_table_lookup(gaa->conv_info, gabuddy->buddy->name);
	msg = http_url_encode(what, TRUE);
	if (conv_info && conv_info->latest_msg_id)
	{
		url_path = conv_info->url_path;
		postdata = g_strdup_printf("host=&id=%" G_GUINT64_FORMAT "&checksum=%s&text=%s&submit=Envoyer&fond=", conv_info->latest_msg_id, conv_info->checksum, msg);
	}
	else
	{
		if (!gabuddy->ref_id)
		{
			purple_debug_error("gayattitude", "ga_message: could not find ref_id for buddy '%s'\n", gabuddy->buddy->name);
			return 1;
		}

		url_path = g_strdup_printf("/html/portrait/message?p=%s&pid=%s&host=&smallheader=&popup=0", gabuddy->buddy->name, gabuddy->ref_id);
		postdata = g_strdup_printf("msg=%s&sendchat=Envoyer+(Shift-Entr%%82e)&fond=&sendmail=0", msg);
	}
	http_post_or_get(gaa->http_handler, HTTP_METHOD_POST, GA_HOSTNAME, url_path,
			postdata, NULL, NULL, FALSE);
	purple_debug_info("gayattitude", "ga_message: sending message to '%s'\n", gabuddy->buddy->name);

	g_free(msg);
	g_free(postdata);
	if (conv_info && conv_info->latest_msg_id)
		conv_info->replied = TRUE;
	else
		g_free(url_path);

	return 0;
}

static void ga_message_send_delayed_cb(GayAttitudeAccount *gaa, GayAttitudeDelayedMessageRequest *delayed_msg)
{
	purple_debug_info("gayattitude", "ga_message: prepare to send delayed message to '%s'\n", delayed_msg->gabuddy->buddy->name);
	ga_message_send_real(gaa, delayed_msg->gabuddy, delayed_msg->what, delayed_msg->flags);
	g_free(delayed_msg);
}

int ga_message_send(GayAttitudeAccount *gaa, GayAttitudeBuddy *gabuddy, const char *what, PurpleMessageFlags flags)
{
	GayAttitudeConversationInfo *conv_info;
	PurpleConversation *conv;

	conv_info = g_hash_table_lookup(gaa->conv_info, gabuddy->buddy->name);
	if (conv_info)
	{
		if (conv_info->replied)
		{
			conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, gabuddy->buddy->name, gaa->account);
			purple_conversation_write(conv, gabuddy->buddy->name, "You cannot reply again to this thread. Wait for a reply or open a new thread.", PURPLE_MESSAGE_SYSTEM, 0);
			return 1;
		}
	}
	else
	{
		if (!gabuddy->ref_id)
		{
			purple_debug_error("gayattitude", "ga_message: ref_id for buddy '%s' is unknown, starting lookup for delayed message\n", gabuddy->buddy->name);

			GayAttitudeDelayedMessageRequest *delayed_msg = g_new0(GayAttitudeDelayedMessageRequest, 1);
			delayed_msg->gaa = gaa;
			delayed_msg->gabuddy = gabuddy;
			delayed_msg->what = g_strdup(what);
			delayed_msg->flags = flags;

			ga_gabuddy_request_info(gaa, gabuddy, FALSE, (GayAttitudeRequestInfoCallbackFunc) ga_message_send_delayed_cb, (gpointer) delayed_msg);
			return 0;
		}
	}
	ga_message_send_real(gaa, gabuddy, what, flags);

	return 0;
}

static void ga_message_extra_info_cb(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	GayAttitudeAccount* gaa = handler->data;
	GayAttitudeMessageExtraInfo* extra_info = (GayAttitudeMessageExtraInfo*)userdata;
	PurpleConversation *conv;
	gchar *message_strtimestamp, *message_url_path, *message_checksum;
	struct tm *message_tm;
	time_t message_timestamp;
	GayAttitudeConversationInfo *conv_info;

	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: looking for extra message info for '%" G_GUINT64_FORMAT "'.\n", extra_info->id);

	doc = htmlReadMemory(response, len, "gayattitude.xml", NULL, 0);
	if (doc == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_message: Unable to parse response (XML Parsing).\n");
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_message: Unable to parse response (XPath context init).\n");
		xmlFreeDoc(doc);
		return;
	}

	message_strtimestamp = g_parsing_quick_xpath_node_content(xpathCtx, "//p[@id='MESSAGESTAMP']", NULL, NULL);
	if (message_strtimestamp)
		g_strstrip(message_strtimestamp);
	message_url_path = g_parsing_quick_xpath_node_content(xpathCtx, "//form[@id='form_reply']", "action", NULL);
	message_checksum = g_parsing_quick_xpath_node_content(xpathCtx, "//form[@id='form_reply']/input[@name='checksum']", "value", NULL);
	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message strtimestamp: %s\n", message_strtimestamp);
	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message url_path: %s\n", message_url_path);
	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message checksum: %s\n", message_checksum);
	/* TODO: fetch history */

	if (message_strtimestamp && message_url_path && message_checksum)
	{
		/* compute timestamp */
		message_tm = g_new0(struct tm, 1);
		strptime(message_strtimestamp, "%d/%m/%y - %H:%M", message_tm);
		message_timestamp = mktime(message_tm);
		g_free(message_tm);

		/* store extra message info */
		conv_info = g_hash_table_lookup(gaa->conv_info, extra_info->conv_name);
		if (conv_info->url_path)
			g_free(conv_info->url_path);
		conv_info->url_path = message_url_path;
		if (conv_info->checksum)
			g_free(conv_info->checksum);
		conv_info->checksum = message_checksum;
		conv_info->timestamp = message_timestamp;

		/* Create conversation if necessary and send message*/
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message timestamp: %u\n", (guint) message_timestamp);
		conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, extra_info->conv_name, gaa->account);
		if (!conv)
			conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, gaa->account, extra_info->conv_name);
		purple_conversation_write(conv, extra_info->sender, extra_info->msg, PURPLE_MESSAGE_RECV, message_timestamp);
	}
	else
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: parsing extra message info for '%" G_GUINT64_FORMAT "' failed.\n", extra_info->id);
}

static void ga_message_received_cb(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	GayAttitudeAccount* gaa = handler->data;
	int i;

	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: looking for received messages.\n");

	doc = htmlReadMemory(response, len, "gayattitude.xml", NULL, 0);
	if (doc == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_message: Unable to parse response (XML Parsing).\n");
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_message: Unable to parse response (XPath context init).\n");
		xmlFreeDoc(doc);
		return;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression((xmlChar*) "//div[@id='LIST']/table[1]/tr", xpathCtx);
	if(xpathObj == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_message: Unable to parse response (XPath evaluation).\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return;
	}
	if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
	{
		/* Print results */
		xmlNodeSetPtr nodes = xpathObj->nodesetval;
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: number of nodes found: %u\n", nodes->nodeNr);

		xmlNode *message_node;
		guint64 new_latest_msg_id = gaa->latest_msg_id;
		for(i = 0; i < nodes->nodeNr; i++)
		{
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message %u\n", i);
			message_node = nodes->nodeTab[i];

			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: plop: %s\n", xmlNodeGetContent(message_node));

			/* Message parsing */
			gchar *message_idstr, *message_sender, *message_content;
			message_idstr = g_parsing_quick_xpath_node_content(xpathCtx, "./td[1]/input", "name", message_node);
			message_sender = g_parsing_quick_xpath_node_content(xpathCtx, "./td[3]/a", NULL, message_node);
			message_content = g_parsing_quick_xpath_node_content(xpathCtx, "./td[4]/a", NULL, message_node);

			/* check if ID is valid */
			guint64 message_id;
			if (message_idstr)
			{
				if (g_str_has_prefix(message_idstr, "msg"))
					message_id = g_ascii_strtoull(message_idstr + 3, NULL, 10);
				g_free(message_idstr);
			}

			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message id: %" G_GUINT64_FORMAT "\n", message_id);
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message sender: %s\n", message_sender);
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message content: %s\n", message_content);

			if (message_id && message_sender && message_content)
			{
				if (message_id > gaa->latest_msg_id)
				{
					guint *conv_count;
					gchar *conv_name, *extra_info_url_path;
					GayAttitudeMessageExtraInfo *msg_extra_info;

					/* Count number of conversation threads with the same user, in order ro differenciate them */
					/* This value is not stored in the GayAttitudeBuddy, as you can talk with someone not in your buddylist */
					conv_count = g_hash_table_lookup(gaa->conv_with_buddy_count, message_sender);
					if (!conv_count)
					{
						conv_count = g_new0(guint, 1);
						g_hash_table_insert(gaa->conv_with_buddy_count, message_sender, conv_count);
					}
					*conv_count += 1;

					/* Generate conversation name */
					conv_name = g_strdup_printf("%s@%u", message_sender, *conv_count);

					/* Store conversation name<->message id association, to allow sending replies to the proper thread */
					GayAttitudeConversationInfo *conv_info = g_new0(GayAttitudeConversationInfo, 1);
					conv_info->replied = FALSE;
					conv_info->latest_msg_id = message_id;
					g_hash_table_insert(gaa->conv_info, conv_name, conv_info);

					/* Fetch extra info and then deliver the received message */
					extra_info_url_path = g_strdup_printf("/html/perso/chat/message?id=%" G_GUINT64_FORMAT "&popup=NONLUS", message_id);
					msg_extra_info = g_new(GayAttitudeMessageExtraInfo, 1);
					msg_extra_info->id = message_id;
					msg_extra_info->sender = message_sender;
					msg_extra_info->msg = message_content;
					msg_extra_info->conv_name = conv_name;
					http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, extra_info_url_path,
						NULL, ga_message_extra_info_cb, msg_extra_info, FALSE);
					g_free(extra_info_url_path);

					if (message_id > new_latest_msg_id)
						new_latest_msg_id = message_id;
				}
				else
					purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: skipped message from %s with id %" G_GUINT64_FORMAT "\n", message_sender, message_id);
			}
			else
				purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: parsing message #%u from received list failed\n", i);
		}

		gaa->latest_msg_id = new_latest_msg_id;
	}

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
}

void ga_message_check_received(GayAttitudeAccount *gaa)
{
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/perso/chat/nonlus/?mode=to",
			NULL, ga_message_received_cb, NULL, FALSE);
}

void ga_message_close_conversation(GayAttitudeAccount *gaa, const gchar *conv_name)
{
	GayAttitudeConversationInfo *conv_info;

	conv_info = g_hash_table_lookup(gaa->conv_info, conv_name);
	if (conv_info)
	{
		if (conv_info->url_path)
			g_free(conv_info->url_path);
		if (conv_info->checksum)
			g_free(conv_info->checksum);
		g_hash_table_remove(gaa->conv_info, conv_name);
	}
	else
		 purple_debug_error("gayattitude", "ga_message: trying to close unexisting conversation '%s'.\n", conv_name);
}

