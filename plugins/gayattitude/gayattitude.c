/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009 Marc Dequènes, Romain Bignon
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

#include <string.h>
#include "gayattitude.h"

PurplePlugin *_gayattitude_plugin = NULL;

static const char *ga_plugin_blist_icon(PurpleAccount *a, PurpleBuddy *b)
{
	return "gayattitude";
}

static void ga_plugin_login(PurpleAccount *account)
{
	GayAttitudeAccount* gaa;

	gaa = ga_account_new(account);
	ga_account_login(gaa);
}

static void ga_plugin_close(PurpleConnection *gc)
{
	g_return_if_fail(gc != NULL);
	g_return_if_fail(gc->proto_data != NULL);

	GayAttitudeAccount* gaa = gc->proto_data;

	ga_account_logout(gaa);
	ga_account_free(gaa);
}

static void ga_plugin_get_info(PurpleConnection *gc, const char *who)
{
	ga_gabuddy_request_info(gc->proto_data, who, TRUE, NULL, NULL);
}

static int ga_plugin_send_im(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags)
{
	GayAttitudeAccount* gaa = gc->proto_data;
	GayAttitudeBuddy *gabuddy;

	gabuddy = ga_gabuddy_find(gaa, who);
	if (!gabuddy)
	{
		purple_debug_error("gayattitude", "ga_plugin: buddy '%s' does not exist\n", who);
		return 1;
	}

	return ga_message_send(gaa, gabuddy, what, flags);
}

static GList *ga_plugin_status_types(PurpleAccount *account)
{
	PurpleStatusType *type;
	GList *types = NULL;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return types;
}

static void ga_plugin_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc = purple_account_get_connection(account);
	//const char *status_id = purple_status_get_id(status);

	g_return_if_fail(gc != NULL);

	if (!purple_status_is_active(status))
		return;

}

static void ga_plugin_keepalive(PurpleConnection *gc)
{
}

static void ga_plugin_buddy_free(PurpleBuddy *buddy)
{
	ga_gabuddy_free(buddy->proto_data);
	buddy->proto_data = NULL;
}

static void ga_plugin_conv_closed(PurpleConnection *gc, const char *who)
{
	GayAttitudeAccount* gaa = gc->proto_data;

	g_hash_table_remove(gaa->conv_latest_msg_id, who);
}

static PurplePluginProtocolInfo ga_plugin_prpl_info =
{
	OPT_PROTO_PASSWORD_OPTIONAL,
	NULL,					/* user_splits */
	NULL,					/* protocol_options */
	NO_BUDDY_ICONS,				/* icon_spec */
	ga_plugin_blist_icon,			/* list_icon */
	NULL,					/* list_emblems */
	NULL,					/* status_text */
	NULL,					/* tooltip_text */
	ga_plugin_status_types,			/* away_states */
	NULL,					/* blist_node_menu */
	NULL,					/* chat_info */
	NULL,					/* chat_info_defaults */
	ga_plugin_login,			/* login */
	ga_plugin_close,			/* close */
	ga_plugin_send_im,			/* send_im */
	NULL,					/* set_info */
	NULL,					/* send_typing */
	ga_plugin_get_info,			/* get_info */
	ga_plugin_set_status,			/* set_status */
	NULL,					/* set_idle */
	NULL,					/* change_passwd */
	NULL,					/* add_buddy */
	NULL,					/* add_buddies */
	NULL,					/* remove_buddy */
	NULL,					/* remove_buddies */
	NULL,					/* add_permit */
	NULL,					/* add_deny */
	NULL,					/* rem_permit */
	NULL,					/* rem_deny */
	NULL,					/* set_permit_deny */
	NULL,					/* join_chat */
	NULL,					/* reject_chat */
	NULL,					/* get_chat_name */
	NULL,					/* chat_invite */
	NULL,					/* chat_leave */
	NULL,					/* chat_whisper */
	NULL,					/* chat_send */
	ga_plugin_keepalive,			/* keepalive */
	NULL,					/* register_user */
	NULL,					/* get_cb_info */
	NULL,					/* get_cb_away */
	NULL,					/* alias_buddy */
	NULL,					/* group_buddy */
	NULL,					/* rename_group */
	ga_plugin_buddy_free,			/* buddy_free */
	ga_plugin_conv_closed,			/* convo_closed */
	purple_normalize_nocase,		/* normalize */
	NULL,					/* set_buddy_icon */
	NULL,					/* remove_group */
	NULL,					/* get_cb_real_name */
	NULL,					/* set_chat_topic */
	NULL,					/* find_blist_chat */
	NULL,					/* roomlist_get_list */
	NULL,					/* roomlist_cancel */
	NULL,					/* roomlist_expand_category */
	NULL,					/* can_receive_file */
	NULL,					/* send_file */
	NULL,					/* new_xfer */
	NULL,					/* offline_message */
	NULL,					/* whiteboard_prpl_ops */
	NULL,					/* send_raw */
	NULL,					/* roomlist_room_serialize */
	NULL,                   		/* unregister_user */
	NULL,                   		/* send_attention */
	NULL,                   		/* get_attention_types */
	sizeof(PurplePluginProtocolInfo),	/* struct_size */
	NULL,                    		/* get_account_text_table */
	NULL,                    		/* initiate_media */
	NULL					 /* can_do_media */
};

static gboolean ga_plugin_load (PurplePlugin *plugin) {

	return TRUE;
}

static PurplePluginInfo ga_plugin_info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,                           /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                          /**< priority       */

	"prpl-gayattitude",                               /**< id             */
	"GayAttitude",                                    /**< name           */
	"1.0",                                            /**< version        */
	"gayattitude Protocol Plugin",                    /**  summary        */
	"www.gayattitude.com chat",                       /**  description    */
	"Marc Dequènes, Romain Bignon",                   /**< author         */
	"http://symlink.me/wiki/minbif",                  /**< homepage       */

	ga_plugin_load,                                   /**< load           */
	NULL,                                             /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	&ga_plugin_prpl_info,                             /**< extra_info     */
	NULL,                                             /**< prefs_info     */
	NULL,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void ga_plugin_init(PurplePlugin *plugin)
{
	PurpleAccountOption *option;
	GHashTable *ui_info = purple_core_get_ui_info();
	const gchar *ui_name = g_hash_table_lookup(ui_info, "version");
	if(!ui_name)
		ui_name = GA_NAME;

	option = purple_account_option_string_new("User-agent", "user-agent", ui_name);
	ga_plugin_prpl_info.protocol_options = g_list_append(ga_plugin_prpl_info.protocol_options, option);

	_gayattitude_plugin = plugin;
}

PURPLE_INIT_PLUGIN(gayattitude, ga_plugin_init, ga_plugin_info);
