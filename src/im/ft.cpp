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

#include <cassert>

#include "im/purple.h"
#include "im/im.h"
#include "im/ft.h"
#include "im/buddy.h"
#include "irc/irc.h"
#include "irc/dcc.h"
#include "core/log.h"
#include "core/config.h"

namespace im {

FileTransfert::FileTransfert()
	: xfer(NULL)
{}

FileTransfert::FileTransfert(PurpleXfer* _xfer)
	: xfer(_xfer)
{}

bool FileTransfert::operator==(const FileTransfert& ft)
{
	return isValid() && ft.isValid() && this->xfer == ft.xfer;
}

string FileTransfert::getFileName() const
{
	assert(isValid());
	return purple_xfer_get_filename(xfer);
}

string FileTransfert::getLocalFileName() const
{
	assert(isValid());
	return purple_xfer_get_local_filename(xfer);
}

size_t FileTransfert::getSize() const
{
	assert(isValid());
	return purple_xfer_get_size(xfer);
}

size_t FileTransfert::getSentBytes() const
{
	assert(isValid());
	return purple_xfer_get_bytes_sent(xfer);
}

bool FileTransfert::isCompleted() const
{
	assert(isValid());
	return purple_xfer_is_completed(xfer);
}

bool FileTransfert::isReceiving() const
{
	assert(isValid());
	return purple_xfer_get_type(xfer) == PURPLE_XFER_RECEIVE;
}

bool FileTransfert::isSending() const
{
	assert(isValid());
	return purple_xfer_get_type(xfer) == PURPLE_XFER_SEND;
}

string FileTransfert::getRemoteUser() const
{
	assert(isValid());
	return purple_xfer_get_remote_user(xfer);
}

Buddy FileTransfert::getBuddy() const
{
	assert(isValid());
	return Buddy(purple_find_buddy(xfer->account, xfer->who));
}

/* STATIC */

void FileTransfert::new_xfer(PurpleXfer* xfer)
{
}

void FileTransfert::destroy(PurpleXfer* xfer)
{
	FileTransfert ft(xfer);
	Purple::getIM()->getIRC()->updateDCC(ft, true);

	if(ft.isCompleted())
	{
		if(ft.isSending())
			b_log[W_INFO|W_SNO] << "File " << ft.getFileName() << " sent to " << ft.getRemoteUser();
		else if(conf.GetSection("file_transfers")->GetItem("dcc")->Boolean() == false)
			b_log[W_INFO|W_SNO] << "File saved as: " << ft.getLocalFileName();
	}
}

void FileTransfert::add_xfer(PurpleXfer* xfer)
{
	irc::Nick* n = NULL;
	irc::IRC* irc = Purple::getIM()->getIRC();
	FileTransfert ft(xfer);
	Buddy buddy = ft.getBuddy();
	if(buddy.isValid())
		n = irc->getNick(Buddy(buddy));
	else
	{
		/* TODO find the chat buddy or someone else. */
	}

	if(ft.isReceiving())
	{
		b_log[W_INFO|W_SNO] << "Starting receiving file " << ft.getFileName() << " from " << ft.getRemoteUser();

		/* Do not send file to IRC user with DCC if this feature is disabled. */
		if(conf.GetSection("file_transfers")->GetItem("dcc")->Boolean() == false)
			return;

		try
		{
			irc->createDCCSend(ft, n);
		}
		catch(irc::DCCListenError &e)
		{
			b_log[W_SNO|W_ERR] << "Unable to listen for DCC, you'll might retrieve yourself the file.";
		}
	}
	else
	{
		b_log[W_INFO|W_SNO] << "Starting sending file " << ft.getFileName() << " to " << ft.getRemoteUser();
	}
}

void FileTransfert::update_progress(PurpleXfer* xfer, double percent)
{
	/* Note: 0 <= percent <= 1 */
	if(conf.GetSection("file_transfers")->GetItem("dcc")->Boolean())
		Purple::getIM()->getIRC()->updateDCC(FileTransfert(xfer));
}

void FileTransfert::cancel_local(PurpleXfer* xfer)
{
	FileTransfert ft(xfer);
	if(ft.isSending())
		b_log[W_SNO|W_ERR] << "Aborted sending file " << ft.getFileName() << " to " << ft.getRemoteUser();
	else
		b_log[W_SNO|W_ERR] << "Aborted receiving file " << ft.getFileName() << " from " << ft.getRemoteUser();
}

void FileTransfert::cancel_remote(PurpleXfer* xfer)
{
	FileTransfert ft(xfer);
	if(ft.isSending())
		b_log[W_SNO|W_ERR] << ft.getRemoteUser() << " aborted receiving file " << ft.getFileName();
	else
		b_log[W_SNO|W_ERR] << ft.getRemoteUser() << " aborted sending file " << ft.getFileName();
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
