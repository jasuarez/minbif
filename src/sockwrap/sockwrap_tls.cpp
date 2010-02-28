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

#include "sockwrap_tls.h"
#include "sock.h"
#include "core/config.h"
#include <sys/socket.h>
#include <cstring>

static void tls_debug_message(int level, const char* message)
{
	b_log[W_SOCK] << "TLS debug: " << message;
}

SockWrapperTLS::SockWrapperTLS(int _recv_fd, int _send_fd) : SockWrapper(_recv_fd, _send_fd)
{
	tls_ok = false;
	tls_handshake = false;

	ConfigSection* c_section = conf.GetSection("aaa");
	if (!c_section->Found())
		throw TLSError::TLSError("Missing section aaa");
	c_section = c_section->GetSection("tls");
	if (!c_section->Found())
		throw TLSError::TLSError("Missing section aaa/tls");

	/* GNUTLS init */
	b_log[W_SOCK] << "Initializing GNUTLS";
	tls_err = gnutls_global_init();
	CheckTLSError();

	/* GNUTLS logging */
	b_log[W_SOCK] << "Setting up GNUTLS logging";
	gnutls_global_set_log_function(tls_debug_message);
	gnutls_global_set_log_level(10);

	b_log[W_SOCK] << "Setting up GNUTLS certificates";
	tls_err = gnutls_certificate_allocate_credentials(&x509_cred);
	CheckTLSError();
	tls_err = gnutls_certificate_set_x509_trust_file(x509_cred,
		c_section->GetItem("trust_file")->String().c_str(),
		GNUTLS_X509_FMT_PEM);
	if (tls_err == 0)
		throw TLSError::TLSError("trust file is empty or does not contain any valid CA certificate");
	else if (tls_err < 0)
		CheckTLSError();
	tls_err = gnutls_certificate_set_x509_key_file(x509_cred,
		c_section->GetItem("cert_file")->String().c_str(),
		c_section->GetItem("key_file")->String().c_str(),
		GNUTLS_X509_FMT_PEM);
	CheckTLSError();

	b_log[W_SOCK] << "Setting up GNUTLS DH params";
	tls_err = gnutls_dh_params_init(&dh_params);
	CheckTLSError();
	tls_err = gnutls_dh_params_generate2(dh_params, 1024);
	CheckTLSError();
	gnutls_certificate_set_dh_params(x509_cred, dh_params);

	b_log[W_SOCK] << "Setting up GNUTLS session";
	tls_err = gnutls_init(&tls_session, GNUTLS_SERVER);
	CheckTLSError();
	tls_err = gnutls_priority_set_direct(tls_session, c_section->GetItem("priority")->String().c_str(), NULL);
	if (tls_err == GNUTLS_E_INVALID_REQUEST)
		throw TLSError::TLSError("syntax error in tls_priority parameter");
	CheckTLSError();
	tls_err = gnutls_credentials_set(tls_session, GNUTLS_CRD_CERTIFICATE, x509_cred);
	CheckTLSError();
	gnutls_transport_set_ptr2(tls_session, (gnutls_transport_ptr_t) recv_fd, (gnutls_transport_ptr_t) send_fd);

	b_log[W_SOCK] << "Starting GNUTLS handshake";
	tls_err = gnutls_handshake (tls_session);
	if (tls_err < 0)
	{
		b_log[W_SOCK] << "TLS handshake failed: " << gnutls_strerror(tls_err);
		throw TLSError::TLSError("TLS initialization failed");
	}
	tls_handshake = true;

	b_log[W_SOCK] << "SSL connection initialized";
	tls_ok = true;
}

void SockWrapperTLS::CheckTLSError()
{
	if (tls_err != GNUTLS_E_SUCCESS)
		throw TLSError::TLSError(gnutls_strerror(tls_err));
}

void SockWrapperTLS::EndSessionCleanup()
{
	sock_ok = false;

	SockWrapper::EndSessionCleanup();

	if (tls_handshake && tls_ok)
		gnutls_bye (tls_session, GNUTLS_SHUT_WR);
	tls_ok = false;

	gnutls_deinit(tls_session);

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_global_deinit();
}

string SockWrapperTLS::Read()
{
	static char buf[1024];
	string sbuf;
	ssize_t r;

	if (!sock_ok || !tls_ok)
		return "";

	r = gnutls_record_recv(tls_session, buf, sizeof buf - 1);
	if (r <= 0)
	{
		sock_ok = false;
		if (r == 0)
		{
			tls_ok = false;
			throw SockError::SockError("Connection reset by peer...");
		}
		else
			throw TLSError::TLSError("Received corrupted data, closing the connection.");
	}
	buf[r] = 0;
	sbuf = buf;

	return sbuf;
}

void SockWrapperTLS::Write(string s)
{
	ssize_t r;

	if (!sock_ok || !tls_ok)
		return;

	r = gnutls_record_send(tls_session, s.c_str(), s.size());
	if (r <= 0)
	{
		sock_ok = false;
		if (r == 0)
		{
			tls_ok = false;
			throw SockError::SockError("Connection reset by peer...");
		}
		else
			throw TLSError::TLSError("Problem while sending data, closing the connection.");
	}
}

