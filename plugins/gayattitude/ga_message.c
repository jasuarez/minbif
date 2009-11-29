
#include "ga_message.h"


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

