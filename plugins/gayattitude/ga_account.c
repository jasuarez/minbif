
#include "ga_account.h"
#include "ga_buddylist.h"


GayAttitudeAccount* ga_account_new(PurpleAccount *account)
{
	GayAttitudeAccount* gaa;

	gaa = g_new0(GayAttitudeAccount, TRUE);
	gaa->account = account;
	gaa->pc = purple_account_get_connection(account);
	gaa->http_handler = http_handler_new(account, gaa);
	account->gc->proto_data = gaa;

	/* basic init */
	gaa->pc->flags |= PURPLE_CONNECTION_NO_NEWLINES;
	purple_connection_set_display_name(gaa->pc, purple_account_get_username(account));

	return gaa;
}

void ga_account_free(GayAttitudeAccount* gaa)
{
	if (gaa->new_messages_check_timer) {
		purple_timeout_remove(gaa->new_messages_check_timer);
	}

	http_handler_free(gaa->http_handler);
	g_free(gaa);
}

static void ga_account_check_changes(GayAttitudeAccount* gaa)
{
	ga_buddylist_check_status(gaa);
	/* TODO: check new messages */
}

static void ga_account_login_cb(HttpHandler* handler, gchar *response, gsize len,
		gpointer userdata)
{
	GayAttitudeAccount *gaa = handler->data;
	purple_connection_update_progress(gaa->pc, "Authenticating", 2, 3);

	if (!g_hash_table_lookup(gaa->http_handler->cookie_table, "cookielogin"))
	{
		purple_connection_error_reason(gaa->pc,
				PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
				"Could not log in GA. (check 'username' and 'password' settings)");
	}
	else
	{
		purple_connection_set_state(gaa->pc, PURPLE_CONNECTED);
		ga_buddylist_update(gaa);
		gaa->new_messages_check_timer = g_timeout_add_seconds(GA_CHECK_INTERVAL,
			(GSourceFunc)ga_account_check_changes, gaa);
	}
}

void ga_account_login(GayAttitudeAccount *gaa)
{
	gchar *postdata;
	const char *username = purple_account_get_username(gaa->account);
	const char *password = purple_account_get_password(gaa->account);

	purple_connection_set_state(gaa->pc, PURPLE_CONNECTING);
	purple_connection_update_progress(gaa->pc, "Connecting", 1, 3);

	gchar* encoded_password = http_url_encode(password, TRUE);
	postdata = g_strdup_printf("login=%s&passw=%s", username, encoded_password);

	http_post_or_get(gaa->http_handler, HTTP_METHOD_POST , GA_HOSTNAME, "/html/login",
			postdata, ga_account_login_cb, NULL, FALSE);

	g_free(encoded_password);
	g_free(postdata);
}

void ga_account_logout(GayAttitudeAccount *gaa)
{
	/* TODO */

	purple_connection_set_state(gaa->pc, PURPLE_DISCONNECTED);
}

