/*
 * libfacebook
 *
 * libfacebook is the property of its developers.  See the COPYRIGHT file
 * for more details.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CC_CONNECTION_H
#define CC_CONNECTION_H

#include "coincoin.h"

/*
 * This is a bitmask.
 */
typedef enum
{
	HTTP_METHOD_GET  = 0x0001,
	HTTP_METHOD_POST = 0x0002,
	HTTP_METHOD_SSL  = 0x0004
} HttpMethod;

typedef struct _HttpConnection HttpConnection;
struct _HttpConnection {
	CoinCoinAccount *fba;
	HttpMethod method;
	gchar *hostname;
	GString *request;
	HttpProxyCallbackFunc callback;
	gpointer user_data;
	char *rx_buf;
	size_t rx_len;
	PurpleProxyConnectData *connect_data;
	PurpleSslConnection *ssl_conn;
	int fd;
	guint input_watcher;
	gboolean connection_keepalive;
	time_t request_time;
};

char* http_url_encode(const char *string, int use_plus);
void http_connection_destroy(HttpConnection *fbconn);
void http_post_or_get(CoinCoinAccount *fba, HttpMethod method,
		const gchar *host, const gchar *url, const gchar *postdata,
		HttpProxyCallbackFunc callback_func, gpointer user_data,
		gboolean keepalive);

#endif /* CC_CONNECTION_H */