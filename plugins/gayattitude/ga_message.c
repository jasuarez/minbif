
#include "ga_message.h"
#include "ga_parsing.h"


static int ga_message_send_real(GayAttitudeAccount *gaa, GayAttitudeBuddy *gabuddy, const char *what, PurpleMessageFlags flags)
{
	purple_debug_info("gayattitude", "ga_message: about to send message to '%s' with ref_id '%s'.\n", gabuddy->buddy->name, gabuddy->ref_id);
	gabuddy = ga_gabuddy_find(gaa, gabuddy->buddy->name);
	if (!gabuddy->ref_id)
	{
		purple_debug_error("gayattitude", "ga_message: could not find ref_id for buddy '%s'\n", gabuddy->buddy->name);
		return 1;
	}

	gchar* url_path = g_strdup_printf("/html/portrait/message?p=%s&pid=%s&host=&smallheader=&popup=0", gabuddy->buddy->name, gabuddy->ref_id);
	gchar* msg = http_url_encode(what, TRUE);
	gchar* postdata = g_strdup_printf("msg=%s&sendchat=Envoyer+(Shift-Entr%%82e)&fond=&sendmail=0", msg);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_POST, GA_HOSTNAME, url_path,
			postdata, NULL, NULL, FALSE);

	g_free(msg);
	g_free(postdata);
	g_free(url_path);
	purple_debug_info("gayattitude", "ga_message: sending message to '%s'\n", gabuddy->buddy->name);

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
	if (!gabuddy->ref_id)
	{
		purple_debug_error("gayattitude", "ga_message: ref_id for buddy '%s' is unknown, starting lookup for delayed message\n", gabuddy->buddy->name);

		GayAttitudeDelayedMessageRequest *delayed_msg = g_new0(GayAttitudeDelayedMessageRequest, TRUE);
		delayed_msg->gaa = gaa;
		delayed_msg->gabuddy = gabuddy;
		delayed_msg->what = g_strdup(what);
		delayed_msg->flags = flags;

		ga_gabuddy_request_info(gaa, gabuddy->buddy->name, FALSE, (GayAttitudeRequestInfoCallbackFunc) ga_message_send_delayed_cb, (gpointer) delayed_msg);
	}
	else
		ga_message_send_real(gaa, gabuddy, what, flags);

	return 0;
}

static void ga_message_received_cb(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	GayAttitudeAccount* gaa = handler->data;
	int i;

	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: Looking for received messages.\n");

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
			gchar *message_idstr, *message_date, *message_sender, *message_content;
			message_idstr = g_parsing_quick_xpath_node_content(xpathCtx, "./td[1]/input", "name", message_node);
			message_date = g_parsing_quick_xpath_node_content(xpathCtx, "./td[2]/nobr/a", NULL, message_node);
			message_sender = g_parsing_quick_xpath_node_content(xpathCtx, "./td[3]/a", NULL, message_node);
			message_content = g_parsing_quick_xpath_node_content(xpathCtx, "./td[4]/a", NULL, message_node);

			/* check if ID is valid */
			guint64 message_id = 0;
			if (g_str_has_prefix(message_idstr, "msg"))
				message_id = g_ascii_strtoull(message_idstr + 3, NULL, 10);
			g_free(message_idstr);

			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message id: %" G_GUINT64_FORMAT "\n", message_id);
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message date: %s\n", message_date);
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message sender: %s\n", message_sender);
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: message content: %s\n", message_content);

			if (message_id && message_date && message_sender && message_content && (message_id > gaa->latest_msg_id))
			{
				PurpleConversation *conv;
				gchar *conv_name;

				conv_name = g_strdup_printf("%s_%" G_GUINT64_FORMAT, message_sender, message_id);
				conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, conv_name, gaa->account);
				if (!conv)
					conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, gaa->account, conv_name);
				/* TODO: convert message_date into time_t */
				purple_conversation_write(conv, message_sender, message_content, PURPLE_MESSAGE_RECV, 0);

				if (message_id > new_latest_msg_id)
				new_latest_msg_id = message_id;
			}
			if (message_id <= gaa->latest_msg_id)
				purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_message: skipped message from %s with id %" G_GUINT64_FORMAT "\n", message_sender, message_id);
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

