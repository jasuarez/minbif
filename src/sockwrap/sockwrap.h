/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon, Marc Dequènes (Duck)
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

#include <fcntl.h>
#include <netdb.h>
#include <string>
#include <vector>
#include "core/log.h"
#include "core/config.h"
#include "core/callback.h"

#if defined(__FreeBSD__) || defined(__FreeBSD) || defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifndef PF_SOCKWRAP_H
#define PF_SOCKWRAP_H

namespace sock
{
	using std::string;
	using std::vector;

	LOGEXCEPTION2(SockError, LogException, W_SOCK);

	class SockWrapper
	{
		ConfigSection* config;
		vector<int> callback_ids;

	public:
		static SockWrapper* Builder(ConfigSection* _config, int _recv_fd, int _send_fd);
		SockWrapper(ConfigSection* _config, int _recv_fd, int _send_fd);
		virtual ~SockWrapper();

		ConfigSection* getConfig() const { return config; }

		virtual string Read() = 0;
		virtual void Write(string s) = 0;
		virtual string GetClientHostname();
		virtual string GetServerHostname();
		virtual int AttachCallback(PurpleInputCondition cond, _CallBack* cb);
		virtual string GetClientUsername();

	protected:
		int recv_fd, send_fd;
		bool sock_ok;

		virtual void EndSessionCleanup();
	};
};

#endif /* PF_SOCKWRAP_H */
