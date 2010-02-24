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

#include "sockwrap.h"
#include "sockwrap_plain.h"
#ifdef HAVE_TLS
#  include "sockwrap_tls.h"
#endif
#include "config.h"

SockWrapper::SockWrapper(int _fd) : fd(_fd)
{
}

SockWrapper::~SockWrapper()
{
}

SockWrapper* SockWrapper::Builder(int _fd)
{
#ifdef HAVE_TLS
	if (conf.GetSection("daemon")->GetItem("security")->String().compare("ssl") == 0)
		return new SockWrapperTLS(_fd);
#endif
	return new SockWrapperPlain(_fd);
}

