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

#ifndef PF_SOCKWRAP_H
#define PF_SOCKWRAP_H

using std::string;

class IRCError
{
	string reason;

public:
	/* temporary silly method with a silly reason */
	IRCError() : reason("unknown error") {}
	IRCError(string _reason) : reason(_reason) {}

	const string Reason() { return reason; }
};

class SockError : public IRCError
{
public:
	SockError(string _reason) : IRCError(_reason) {}
};

class SockWrapper
{
public:
	static SockWrapper* Builder(int _fd);
	SockWrapper(int _fd);
	virtual ~SockWrapper();

	virtual string Read() = 0;
	virtual string GetClientHostname();
	virtual string GetServerHostname();

protected:
	int fd;
};

#endif /* PF_SOCKWRAP_H */
