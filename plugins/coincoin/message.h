/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2011 Romain Bignon
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

#ifndef CC_MESSAGE_H
#define CC_MESSAGE_H

#include "coincoin.h"

#define CC_LAST_MESSAGE_MAX 80

typedef struct _CoinCoinMessage CoinCoinMessage;

struct _CoinCoinMessage
{
	gchar* message;
	gchar* info;
	gchar* from;
	time_t timestamp;
	unsigned ref;
	gboolean multiple;
	gint64 id;
};

gchar* coincoin_convert_message(CoinCoinAccount* cca, const char* msg);
void coincoin_parse_message(HttpHandler* handler, gchar* response, gsize len, gpointer userdata);
xmlnode* coincoin_xmlparse(gchar* response, gsize len);
void coincoin_message_free(CoinCoinMessage* msg);

#endif /* CC_MESSAGE_H */
