/*
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

#include "im.h"
#include "purple_handler.h"
#include "purple.h"
#include "../util.h"
#include "../log.h"
#include "../version.h"

namespace im
{

PurpleEventLoopUiOps PurpleHandler::eventloop_ops =
{
	/* timeout_add */    g_timeout_add,
	/* timeout_remove */ g_source_remove,
	/* input_add */      glib_input_add,
	/* input_remove */   g_source_remove,
	/* input_get_error*/ NULL,
	/* timeout_add_seconds */ NULL,
	NULL, NULL, NULL
};

void PurpleHandler::debug(PurpleDebugLevel level, const char *category, const char *args)
{
	b_log[W_DEBUG] << "[" << category << "] " << args;
}

PurpleDebugUiOps PurpleHandler::debug_ops =
{
        PurpleHandler::debug,
        NULL, //finch_debug_is_enabled,

        /* padding */
        NULL,
        NULL,
        NULL,
        NULL
};

void PurpleHandler::debug_init()
{
	purple_debug_set_ui_ops(&debug_ops);
}

void PurpleHandler::bitlbee_prefs_init()
{
	purple_prefs_add_none("/bitlbee");

	purple_prefs_add_string("/bitlbee/password", "");
}

GHashTable *PurpleHandler::ui_info = NULL;
GHashTable *PurpleHandler::bitlbee_ui_get_info(void)
{
        if (ui_info == NULL) {
                ui_info = g_hash_table_new(g_str_hash, g_str_equal);

                g_hash_table_insert(ui_info, (void*)"name",         (void*)BITLBEE_VERSION_NAME);
                g_hash_table_insert(ui_info, (void*)"version",      (void*)BITLBEE_VERSION);
                g_hash_table_insert(ui_info, (void*)"website",      (void*)"http://symlink.me/wiki/bitlbee");
                g_hash_table_insert(ui_info, (void*)"dev_website",  (void*)"http://symlink.me/projects/show/bitlbee2");
        }

        return ui_info;
}


PurpleCoreUiOps PurpleHandler::core_ops =
{
	PurpleHandler::bitlbee_prefs_init,
        PurpleHandler::debug_init,
        NULL,//gnt_ui_init,
        NULL,//bitlbee_quit,
        PurpleHandler::bitlbee_ui_get_info,

        /* padding */
        NULL,
        NULL,
        NULL
};

void PurpleHandler::Init()
{
	purple_core_set_ui_ops(&core_ops);
	purple_eventloop_set_ui_ops(&eventloop_ops);
}

}; /* namespace im */
