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
#include "../lib/http.h"

#define CC_NAME "libpurple (coincoin)"
#define CC_DEFAULT_HOSTNAME "linuxfr.org"
#define CC_DEFAULT_BOARD "/board/remote.xml"
#define CC_DEFAULT_POST "/board/add.html"
#define CC_CHECK_INTERVAL 30

typedef struct _CoinCoinAccount CoinCoinAccount;

struct _CoinCoinAccount {
	PurpleAccount *account;
	PurpleConnection *pc;
	HttpHandler* http_handler;
	GSList* messages;
	guint new_messages_check_timer;

	gchar* hostname;
};

#endif /* CC_COINCOIN_H */
