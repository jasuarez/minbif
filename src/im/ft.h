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

#ifndef IM_FT_H
#define IM_FT_H

#include <libpurple/purple.h>

namespace im {

	class FileTransfert
	{
		static PurpleXferUiOps ui_ops;

		static void new_xfer(PurpleXfer* xfer);
		static void destroy(PurpleXfer* xfer);
		static void add_xfer(PurpleXfer* xfer);
		static void update_progress(PurpleXfer* xfer, double percent);
		static void cancel_local(PurpleXfer* xfer);
		static void cancel_remote(PurpleXfer* xfer);
	public:

		static void init();
		static void uninit();
	};

};

#endif /* IM_FT_H */
