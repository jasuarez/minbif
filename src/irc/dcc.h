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

#ifndef IRC_DCC_H
#define IRC_DCC_H

#include <exception>
#include <string>
#include <stdio.h>

#include "im/ft.h"

namespace irc {

	using std::string;
	class Nick;

	/* Exceptions */
	class DCCListenError : public std::exception {};

	class DCC
	{
	public:

		virtual im::FileTransfert getFileTransfert() const = 0;
		virtual void updated(bool destroy) = 0;
		virtual bool isFinished() const = 0;
		virtual Nick* getPeer() const = 0;
		virtual void setPeer(Nick* n) = 0;
	};

	class DCCSend : public DCC
	{
		/** The DCC class used to send a file to a IRC user.
		 *
		 * An instance of this class is created when a file transfert starts
		 * on im->minbif. It creates a DCC server on a random port.
		 *
		 * TODO set a specific range, or allow admin to set it.
		 *
		 * When IRC user is connected on server, try to open the file that
		 * libpurple is currently writting. If success, read 512 bytes and
		 * send it to IRC user with DCC connection.
		 *
		 * Everytimes we receive an ACK from IRC user on DCC connection, or
		 * when libpurple sends us a percentage update, retry to send data
		 * to DCC user.
		 *
		 * When im->minbif transfert is finished, the minbif->irc transfert
		 * isn't finished. So the 'ft' reference is removed, and this is the
		 * read callback which ask to retry to send data to DCC user from
		 * file.
		 *
		 * The file descriptor keeps open.
		 */

		static const unsigned TIMEOUT = 5*60;

		im::FileTransfert ft;
		size_t total_size;
		time_t start_time;
		string filename;
		string local_filename;

		Nick* sender;
		Nick* receiver;
		PurpleNetworkListenData* listen_data;
		int watcher;
		int fd;
		unsigned port;
		bool finished;

		size_t bytes_sent;
		FILE* fp;
		guint rxlen;
		guchar* rxqueue;

		static void listen_cb(int sock, void* data);
		static void connected(gpointer data, int source, PurpleInputCondition cond);
		static void dcc_read(gpointer data, int source, PurpleInputCondition cond);
		void dcc_send();
		void deinit();

	public:
		DCCSend(const im::FileTransfert& ft, Nick* sender, Nick* receiver);
		~DCCSend();

		im::FileTransfert getFileTransfert() const { return ft; }
		void updated(bool destroy);
		bool isFinished() const { return finished; }

		Nick* getPeer() const { return sender; }
		void setPeer(Nick* n) { sender = n; }
	};

};

#endif /* IRC_DCC_H */
