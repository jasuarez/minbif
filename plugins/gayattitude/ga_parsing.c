
#include "ga_parsing.h"


/* if attrib == NULL, then get text() */
gchar* g_parsing_quick_xpath_node_content(xmlXPathContextPtr xpathCtx, gchar *xpath_expr, gchar *attrib, xmlNodePtr base_node)
{
	xmlXPathObjectPtr xpathObj;
	xmlChar *xmlstr;
	gchar *str = NULL;

	if (base_node)
	{
		/* (enforce current search node and look for message details) */
		xpathCtx->node = base_node;
	}

	xpathObj = xmlXPathEvalExpression((xmlChar*) xpath_expr, xpathCtx);
	if (!xpathObj)
	{
		purple_debug(PURPLE_DEBUG_ERROR, "gayattitude", "ga_parsing: Unable to parse response (XPath evaluation).\n");
		return NULL;
	}

	if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
	{
		if (attrib)
			xmlstr = xmlGetProp(xpathObj->nodesetval->nodeTab[0], (xmlChar*) attrib);
		else
			xmlstr = xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		str = g_strdup((gchar*) xmlstr);
		xmlFree(xmlstr);
	}

	xmlXPathFreeObject(xpathObj);

	return str;
}

