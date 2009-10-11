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
#include <stdint.h>

#include "im/ft.h"

class _CallBack;

namespace irc {

	using std::string;
	class Nick;

	/* Exceptions */
	class DCCListenError : public std::exception {};
	class DCCGetError : public std::exception {};

	class DCC
	{
	public:

		virtual ~DCC() {}

		virtual im::FileTransfert getFileTransfert() const = 0;
		virtual void updated(bool destroy) = 0;
		virtual bool isFinished() const = 0;
		virtual Nick* getPeer() const = 0;
		virtual void setPeer(Nick* n) = 0;
	};

	class DCCServer : public DCC
	{
	protected:
		static const time_t TIMEOUT = 5*60;

		string type;
		string filename;
		size_t total_size;

		time_t start_time;
		Nick* sender;
		Nick* receiver;
		PurpleNetworkListenData* listen_data;
		int watcher;
		int fd;
		unsigned port;
		bool finished;

		static void listen_cb(int sock, void* data);
		static void connected(gpointer data, int source, PurpleInputCondition cond);
		static void dcc_read_cb(gpointer data, int source, PurpleInputCondition cond);

		virtual void deinit();
		virtual void dcc_read(int source) = 0;
	public:

		DCCServer(string type, string filename, size_t total_size, Nick* sender, Nick* receiver);
		~DCCServer();

		bool isFinished() const { return finished; }

		Nick* getPeer() const { return sender; }
		void setPeer(Nick* n) { sender = n; }
	};

	class DCCSend : public DCCServer
	{
		/** The DCC class used to send a file to a IRC user.
		 *
		 * An instance of this class is created when a file transfert starts
		 * on im->minbif. It creates a DCC server on a random port.
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
		im::FileTransfert ft;

		string local_filename;

		size_t bytes_sent;
		FILE* fp;
		guint rxlen;
		guchar* rxqueue;

		virtual void deinit();
		virtual void dcc_read(int source);
		void dcc_send();

	public:
		DCCSend(const im::FileTransfert& ft, Nick* sender, Nick* receiver);
		~DCCSend();

		im::FileTransfert getFileTransfert() const { return ft; }
		void updated(bool destroy);
	};

	class DCCChat : public DCCServer
	{
		virtual void deinit();
		virtual void dcc_read(int source);
	public:

		DCCChat(Nick* sender, Nick* receiver);
		~DCCChat();

		im::FileTransfert getFileTransfert() const { return im::FileTransfert(); }
		void updated(bool destroy);
		void dcc_send(string buf);
	};

	class DCCGet : public DCC
	{
		Nick* from;
		string filename;
		_CallBack* callback;

		bool finished;
		int sock;
		int watcher;
		FILE* fp;
		ssize_t bytes_received;
		ssize_t total_size;

		void deinit();
		static void dcc_read(gpointer data, int source, PurpleInputCondition cond);
	public:

		/** Get a file from a user, and call a method when it is finished. */
		DCCGet(Nick* from, string filename, uint32_t addr, uint16_t port, ssize_t size, _CallBack* callback);
		~DCCGet();

		static bool parseDCCSEND(string line, string* filename, uint32_t* addr, uint16_t* port, ssize_t* size);

		virtual im::FileTransfert getFileTransfert() const { return im::FileTransfert(); }
		virtual void updated(bool destroy);
		virtual bool isFinished() const { return finished; }
		virtual Nick* getPeer() const { return from; }
		virtual void setPeer(Nick* n);
	};


};

#endif /* IRC_DCC_H */
