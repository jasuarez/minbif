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
#include "core/log.h"
#include <sys/socket.h>
#include <cstring>

static void tls_debug_message(int level, const char* message)
{
	b_log[W_DEBUG] << "TLS debug: " << message;
}

SockWrapperTLS::SockWrapperTLS(int _recv_fd, int _send_fd) : SockWrapper(_recv_fd, _send_fd)
{
	/* GNUTLS init */
	int tls_err = gnutls_global_init();
	if (tls_err != GNUTLS_E_SUCCESS)
	{
		b_log[W_ERR] << "Cannot initialize the GNUTLS library: " << gnutls_strerror(tls_err);
		throw TLSError::TLSError("TLS initialization failed");
	}

	/* GNUTLS logging */
	gnutls_global_set_log_function(tls_debug_message);
	gnutls_global_set_log_level(10);

	gnutls_certificate_allocate_credentials(&x509_cred);
	gnutls_certificate_set_x509_trust_file(x509_cred,
		conf.GetSection("aaa")->GetItem("tls_ca_file")->String().c_str(),
		GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_x509_key_file(x509_cred,
		conf.GetSection("aaa")->GetItem("tls_cert_file")->String().c_str(),
		conf.GetSection("aaa")->GetItem("tls_key_file")->String().c_str(),
		GNUTLS_X509_FMT_PEM);

	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_generate2(dh_params, 1024);
	gnutls_certificate_set_dh_params(x509_cred, dh_params);

	gnutls_priority_init(&priority_cache, "NORMAL", NULL);

	gnutls_init(&tls_session, GNUTLS_SERVER);
	gnutls_priority_set(tls_session, priority_cache);
	gnutls_credentials_set(tls_session, GNUTLS_CRD_CERTIFICATE, x509_cred);
	gnutls_transport_set_ptr2(tls_session, (gnutls_transport_ptr_t) recv_fd, (gnutls_transport_ptr_t) send_fd);

	tls_err = gnutls_handshake (tls_session);
	if (tls_err < 0)
	{
		EndSessionCleanup();

		b_log[W_ERR] << "TLS handshake failed: " << gnutls_strerror(tls_err);
		throw TLSError::TLSError("TLS initialization failed");
	}
}

SockWrapperTLS::~SockWrapperTLS()
{
	gnutls_bye (tls_session, GNUTLS_SHUT_WR);
}

void SockWrapperTLS::EndSessionCleanup()
{
	SockWrapper::EndSessionCleanup();

	gnutls_deinit(tls_session);

	gnutls_certificate_free_credentials(x509_cred);
	gnutls_priority_deinit(priority_cache);
	gnutls_global_deinit();
}

string SockWrapperTLS::Read()
{
	static char buf[1024];
	string sbuf;
	ssize_t r;

	r = gnutls_record_recv(tls_session, buf, sizeof buf - 1);
	if (r == 0)
		throw SockError::SockError("Connection reset by peer...");
	else if (r < 0)
		throw TLSError::TLSError("Received corrupted data, closing the connection.");
	buf[r] = 0;
	sbuf = buf;

	return sbuf;
}

void SockWrapperTLS::Write(string s)
{
	ssize_t r;

	r = gnutls_record_send(tls_session, s.c_str(), s.size());
	if (r == 0)
		throw SockError::SockError("Connection reset by peer...");
	else if (r < 0)
		throw TLSError::TLSError("Problem while sending data, closing the connection.");
}

