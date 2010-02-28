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
#include <gnutls/gnutls.h>

#ifndef PF_SOCKWRAP_TLS_H
#define PF_SOCKWRAP_TLS_H

class TLSError : public IRCError
{
public:
	TLSError(string _reason) : IRCError("[TLS] " + _reason) {}
};

class SockWrapperTLS : public SockWrapper
{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_dh_params_t dh_params;
	gnutls_session_t tls_session;

	int tls_err;

	void EndSessionCleanup();
	void CheckTLSError();

public:
	SockWrapperTLS(int _recv_fd, int _send_fd);
	~SockWrapperTLS();

	string Read();
	void Write(string s);
};

#endif /* PF_SOCKWRAP_TLS_H */
