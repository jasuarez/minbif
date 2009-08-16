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

#include "im/ft.h"
#include "../log.h"

namespace im {

/* STATIC */

void FileTransfert::new_xfer(PurpleXfer* xfer)
{
}

void FileTransfert::destroy(PurpleXfer* xfer)
{
	if(purple_xfer_is_completed(xfer))
		b_log[W_INFO|W_SNO] << "File saved as: " << purple_xfer_get_local_filename(xfer);
}

void FileTransfert::add_xfer(PurpleXfer* xfer)
{
	b_log[W_SNO] << "Starting receiving file " << purple_xfer_get_filename(xfer);
}

void FileTransfert::update_progress(PurpleXfer* xfer, double percent)
{
	/* Note: 0 <= percent <= 1 */
}

void FileTransfert::cancel_local(PurpleXfer* xfer)
{
	b_log[W_SNO|W_ERR] << "Aborted receiving file " << purple_xfer_get_filename(xfer) << " from " << purple_xfer_get_remote_user(xfer);
}

void FileTransfert::cancel_remote(PurpleXfer* xfer)
{
	b_log[W_SNO|W_ERR] << purple_xfer_get_remote_user(xfer) << " aborted sending file " << purple_xfer_get_filename(xfer);
}


PurpleXferUiOps FileTransfert::ui_ops =
{
	new_xfer,
	destroy,
	add_xfer,
	update_progress,
	cancel_local,
	cancel_remote,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

void FileTransfert::init()
{
	purple_xfers_set_ui_ops(&ui_ops);
}

void FileTransfert::uninit()
{
	purple_xfers_set_ui_ops(NULL);
}

}; /* ns im */
