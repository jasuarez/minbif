#ifndef GA_GAPARSING_H
#define GA_GAPARSING_H

#include <purple.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/HTMLparser.h>

#define GA_HOSTNAME "www.gayattitude.com"
#define GA_HOSTNAME_PERSO "perso.gayattitude.com"

gchar* g_parsing_quick_xpath_node_content(xmlXPathContextPtr xpathCtx, gchar *xpath_expr, gchar *attrib, xmlNodePtr base_node);

#endif /* GA_GAPARSING_H */
