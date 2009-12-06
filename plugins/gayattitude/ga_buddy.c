
#include "ga_buddy.h"
#include "ga_parsing.h"


GayAttitudeBuddy *ga_gabuddy_get_from_buddy(PurpleBuddy *buddy, gboolean create)
{
	GayAttitudeBuddy *gabuddy;

	if (!buddy)
		return NULL;

	purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Looking for GayAttitudeBuddy for buddy '%s'.\n", buddy->name);

	gabuddy = buddy->proto_data;
	if (!gabuddy && create)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Creating GayAttitudeBuddy for buddy '%s'.\n", buddy->name);
		gabuddy = g_new0(GayAttitudeBuddy, TRUE);
		gabuddy->buddy = buddy;
		gabuddy->ref_id = NULL;
		buddy->proto_data = gabuddy;
	}

	return gabuddy;
}

GayAttitudeBuddy *ga_gabuddy_find(GayAttitudeAccount *gaa, const gchar *buddyname)
{
	PurpleBuddy *buddy;
	GayAttitudeBuddy *gabuddy;

	buddy = purple_find_buddy(gaa->account, buddyname);
	if (!buddy)
		return NULL;

	gabuddy = ga_gabuddy_get_from_buddy(buddy, TRUE);

	return gabuddy;
}

GayAttitudeBuddy *ga_gabuddy_new(GayAttitudeAccount *gaa, const gchar *buddyname)
{
	PurpleBuddy *buddy;

	buddy = purple_buddy_new(gaa->account, buddyname, NULL);
	return ga_gabuddy_get_from_buddy(buddy, TRUE);
}

void ga_gabuddy_free(GayAttitudeBuddy *gabuddy)
{
	if (!gabuddy)
		return;

	g_free(gabuddy);
}

static void ga_gabuddy_parse_info_cb(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	GayAttitudeAccount *gaa = handler->data;
	GayAttitudeBuddyInfoRequest *request = userdata;

	purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddy: Fetching info for '%s'.\n", request->gabuddy->buddy->name);

	doc = htmlReadMemory(response, len, "gayattitude.xml", NULL, 0);
	if (doc == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Unable to parse response (XML Parsing).\n");
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Unable to parse response (XPath context init).\n");
		xmlFreeDoc(doc);
		return;
	}

	xmlNode *info_node;

	/* Search internal Ref ID */
	if (!request->gabuddy->ref_id)
	{
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddy: Fetching missing ref_id for '%s'.\n", request->gabuddy->buddy->name);

		xpathObj = xmlXPathEvalExpression((xmlChar*) "//input[@type='hidden' and @name='ref_id']", xpathCtx);
		if(xpathObj == NULL)
		{
			purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Unable to parse response (XPath evaluation).\n");
			xmlXPathFreeContext(xpathCtx);
			xmlFreeDoc(doc);
			return;
		}
		if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
		{
			info_node = xpathObj->nodesetval->nodeTab[0];
			request->gabuddy->ref_id  = (gchar*) xmlGetProp(info_node, (xmlChar*) "value");
			purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddy: Found ref_id for '%s': %s.\n", request->gabuddy->buddy->name, request->gabuddy->ref_id);
		}
		xmlXPathFreeObject(xpathObj);
	}

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
			purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Unable to parse response (XPath evaluation).\n");
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
			purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddy: Unable to parse response (XPath evaluation).\n");
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

		purple_notify_userinfo(gaa->pc, request->gabuddy->buddy->name, user_info, NULL, NULL);
		purple_notify_user_info_destroy(user_info);
	}

	/* Cleanup */
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);

	/* Chained Callback */
	if (request->callback)
	{
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddy: Calling callback after info for '%s' was retrieved\n", request->gabuddy->buddy->name);
		request->callback(gaa, request->callback_data);
	}
}

void ga_gabuddy_request_info(GayAttitudeAccount* gaa, const char *who, gboolean advertise, GayAttitudeRequestInfoCallbackFunc callback, gpointer callback_data)
{
	gchar *url_path;
	GayAttitudeBuddyInfoRequest *request = g_new0(GayAttitudeBuddyInfoRequest, TRUE);
	GayAttitudeBuddy *gabuddy;

	gabuddy = ga_gabuddy_find(gaa, who);
	if (!gabuddy)
		return;

	url_path = g_strdup_printf("/%s", who);
	request->gabuddy = gabuddy;
	request->advertise = advertise;
	request->callback = callback;
	request->callback_data = callback_data;
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME_PERSO, url_path,
			NULL, ga_gabuddy_parse_info_cb, (gpointer) request, FALSE);
	g_free(url_path);
}

