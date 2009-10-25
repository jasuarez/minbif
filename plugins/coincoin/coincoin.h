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

#ifndef CC_COINCOIN_H
#define CC_COINCOIN_H

#include <purple.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define CC_DEFAULT_HOSTNAME "linuxfr.org"
#define CC_DEFAULT_BOARD "/board/remote.xml"
#define CC_DEFAULT_POST "/board/add.html"
#define CC_CHECK_INTERVAL 30
#define CC_LAST_MESSAGE_MAX 10

typedef struct _CoinCoinAccount CoinCoinAccount;

typedef void (*HttpProxyCallbackFunc)(CoinCoinAccount *fba, gchar *data, gsize data_len, gpointer user_data);

struct _CoinCoinAccount {
	PurpleAccount *account;
	PurpleConnection *pc;
	GSList *conns; /**< A list of all active HttpConnections */
	GSList *dns_queries;
	GHashTable *cookie_table;
	gint64 last_messages[CC_LAST_MESSAGE_MAX];
	GHashTable *hostname_ip_cache;
	guint new_messages_check_timer;

	gchar* hostname;
};

#endif /* CC_COINCOIN_H */
