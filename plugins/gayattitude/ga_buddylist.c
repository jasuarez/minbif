
#include "ga_buddylist.h"
#include "ga_buddy.h"
#include "ga_parsing.h"
#include <string.h>

static void ga_buddylist_parse_cb(HttpHandler* handler, gchar* response, gsize len, gpointer userdata)
{
	htmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	GayAttitudeAccount* gaa = handler->data;
	gchar* group_name = userdata;

	doc = htmlReadMemory(response, len, "gayattitude.xml", NULL, 0);
	if (doc == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddylist: Unable to parse response (XML Parsing).\n");
		return;
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddylist: Unable to parse response (XPath context init).\n");
		xmlFreeDoc(doc);
		return;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression((xmlChar*) "//div[@id='ANNUAIRE']/div[@id='RESULTATS']/div", xpathCtx);
	if(xpathObj == NULL)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_buddylist: Unable to parse response (XPath evaluation).\n");
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return;
	}
	if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
	{
		/* Print results */
		xmlNodeSetPtr nodes = xpathObj->nodesetval;
		purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddylist: number of nodes found: %i\n", nodes->nodeNr);

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
		xmlChar *xmlprop = NULL;
		xmlNode *contact_node;
		for(i = 0; i < nodes->nodeNr; i++)
		{
			contact_node = nodes->nodeTab[i];
			xmlprop = xmlGetProp(contact_node, (xmlChar*) "class");

			/* look for contacts */
			if (xmlprop && g_str_has_prefix((gchar*) xmlprop, "ITEM"))
			{
				gchar *contact_name;
				GayAttitudeBuddy *gabuddy;

				contact_name = g_parsing_quick_xpath_node_content(xpathCtx, "./div[@class='ITEM2']/div[@class='pseudo']/a", NULL, contact_node);
				if (contact_name)
				{
					purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddylist: found buddy from server: %s\n", contact_name);

					gabuddy = ga_gabuddy_find(gaa, contact_name);
					if (!gabuddy)
					{
						gabuddy = ga_gabuddy_new(gaa, contact_name);
						purple_blist_add_buddy(gabuddy->buddy, NULL, group, NULL);
						purple_debug(PURPLE_DEBUG_INFO, "gayattitude", "ga_buddylist: added missing buddy: %s\n", contact_name);
					}
					if (strstr((char*) xmlprop, "ITEMONLINE"))
						purple_prpl_got_user_status(gaa->account, contact_name, "available", NULL);
					else
						purple_prpl_got_user_status(gaa->account, contact_name, "offline", NULL);
					g_free(contact_name);
				}
			}
			xmlFree(xmlprop);
		}
	}

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
}

void ga_buddylist_update(GayAttitudeAccount* gaa)
{
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-checklist",
			NULL, ga_buddylist_parse_cb, g_strdup("Checklist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-friendlist",
			NULL, ga_buddylist_parse_cb, g_strdup("Friendlist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-hotlist",
			NULL, ga_buddylist_parse_cb, g_strdup("Hotlist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-blogolist",
			NULL, ga_buddylist_parse_cb, g_strdup("Blogolist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-blacklist",
			NULL, ga_buddylist_parse_cb, g_strdup("Blacklist"), FALSE);
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-whitelist",
			NULL, ga_buddylist_parse_cb, g_strdup("Whitelist"), FALSE);
}

void ga_buddylist_check_status(GayAttitudeAccount* gaa)
{
	http_post_or_get(gaa->http_handler, HTTP_METHOD_GET, GA_HOSTNAME, "/html/annuaire/liste?liste=contacts-online",
			NULL, ga_buddylist_parse_cb, NULL, FALSE);
}

