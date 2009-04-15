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

#include <libpurple/purple.h>

#include "purple.h"
#include "im.h"
#include "../util.h"
#include "../version.h"
#include "../log.h"

IM* Purple::im = NULL;

static PurpleEventLoopUiOps eventloop_wg_ops = {
	/* timeout_add */    g_timeout_add,
	/* timeout_remove */ g_source_remove,
	/* input_add */      glib_input_add,
	/* input_remove */   g_source_remove,
	/* input_get_error*/ NULL,
	/* timeout_add_seconds */ NULL,
	NULL, NULL, NULL
};

static GHashTable *ui_info = NULL;
static GHashTable *bitlbee_ui_get_info(void)
{
        if (ui_info == NULL) {
                ui_info = g_hash_table_new(g_str_hash, g_str_equal);

                g_hash_table_insert(ui_info, (void*)"name",         (void*)"bitlbee");
                g_hash_table_insert(ui_info, (void*)"version",      (void*)BITLBEE_VERSION);
                g_hash_table_insert(ui_info, (void*)"website",      (void*)"http://symlink.me/wiki/bitlbee");
                g_hash_table_insert(ui_info, (void*)"dev_website",  (void*)"http://symlink.me/projects/show/bitlbee2");
        }

        return ui_info;
}

static void bitlbee_prefs_init()
{
	purple_prefs_add_none("/bitlbee");

	purple_prefs_add_string("/bitlbee/password", "");
}

static PurpleCoreUiOps core_ops =
{
        bitlbee_prefs_init,
        NULL,//debug_init,
        NULL,//gnt_ui_init,
        NULL,//bitlbee_quit,
        bitlbee_ui_get_info,

        /* padding */
        NULL,
        NULL,
        NULL
};

void Purple::Init(IM* im)
{
	purple_util_set_user_dir(im->getUserPath().c_str());
	purple_core_set_ui_ops(&core_ops);
	purple_eventloop_set_ui_ops(&eventloop_wg_ops);

	if (!purple_core_init(BITLBEE_VERSION_NAME))
	{
		b_log[W_ERR] << "Initialization of the Purple core failed.";
		throw PurpleError();
	}
}
