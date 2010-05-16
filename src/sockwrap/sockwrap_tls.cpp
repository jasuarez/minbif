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
#include <sys/socket.h>
#include <cstring>
#include "gnutls/x509.h"

namespace sock
{

static void tls_debug_message(int level, const char* message)
{
	b_log[W_SOCK] << "TLS debug: " << message;
}

SockWrapperTLS::SockWrapperTLS(ConfigSection* _config, int _recv_fd, int _send_fd)
	: SockWrapper(_config, _recv_fd, _send_fd)
{
	tls_ok = false;
	trust_check = false;

	ConfigSection* c_section = getConfig()->GetSection("tls");
	if (!c_section->Found())
		throw TLSError("Missing section <inetd|daemon>/tls");

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
	string trust_file = c_section->GetItem("trust_file")->String();
	if (trust_file != " ")
	{
		tls_err = gnutls_certificate_set_x509_trust_file(x509_cred,
			trust_file.c_str(), GNUTLS_X509_FMT_PEM);
		if (tls_err == GNUTLS_E_SUCCESS)
			throw TLSError("trust file is empty or does not contain any valid CA certificate");
		else if (tls_err < 0)
			CheckTLSError();
		trust_check = true;
	}
	string crl_file = c_section->GetItem("crl_file")->String();
	if (trust_check && crl_file != " ")
	{
		tls_err = gnutls_certificate_set_x509_crl_file(x509_cred,
			crl_file.c_str(), GNUTLS_X509_FMT_PEM);
		if (tls_err == GNUTLS_E_SUCCESS)
			b_log[W_WARNING] << "trust file is empty or does not contain any valid CA certificate";
		else if (tls_err < 0)
			CheckTLSError();
	}
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
		throw TLSError("syntax error in tls_priority parameter");
	CheckTLSError();
	tls_err = gnutls_credentials_set(tls_session, GNUTLS_CRD_CERTIFICATE, x509_cred);
	CheckTLSError();
	gnutls_transport_set_ptr2(tls_session, (gnutls_transport_ptr_t) recv_fd, (gnutls_transport_ptr_t) send_fd);
	if (trust_check)
	{
		/* client auth would need GNUTLS_CERT_REQUIRE but check is enforced in GetClientUsername() instead */
		gnutls_certificate_server_set_request(tls_session, GNUTLS_CERT_REQUEST);
	}

	ProcessTLSHandshake();

	b_log[W_SOCK] << "SSL connection initialized";
	tls_ok = true;
}

void SockWrapperTLS::ProcessTLSHandshake()
{
	b_log[W_SOCK] << "Starting GNUTLS handshake";
	tls_handshake = false;
	tls_err = -1;
	while (tls_err != GNUTLS_E_SUCCESS)
	{
		tls_err = gnutls_handshake (tls_session);
		if (gnutls_error_is_fatal(tls_err))
		{
			b_log[W_SOCK] << "TLS handshake failed: " << gnutls_strerror(tls_err);
			throw TLSError("TLS initialization failed");
		}

		if (tls_err != GNUTLS_E_SUCCESS)
			usleep(100);
	}
	tls_handshake = true;
}

void SockWrapperTLS::CheckTLSError()
{
	if (tls_err != GNUTLS_E_SUCCESS)
		throw TLSError(gnutls_strerror(tls_err));
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
	string sbuf = "";
	ssize_t r = GNUTLS_E_AGAIN;

	if (!sock_ok || !tls_ok || !tls_handshake)
		return "";

	while (tlserr_again(r))
	{
		r = gnutls_record_recv(tls_session, buf, sizeof buf - 1);
		if (r <= 0)
		{
			sock_ok = false;
			if (r == 0)
			{
				tls_ok = false;
				throw SockError("Connection reset by peer...");
			}
			else if (gnutls_error_is_fatal(r))
			{
				tls_err = r;
				CheckTLSError();
			}
			else if (r == GNUTLS_E_REHANDSHAKE)
			{
				ProcessTLSHandshake();
				return "";
			}
			else
				usleep(100);
		}
		else
		{
			buf[r] = 0;
			sbuf = buf;
		}
	}

	return sbuf;
}

void SockWrapperTLS::Write(string s)
{
	ssize_t r = GNUTLS_E_AGAIN;

	if (!sock_ok || !tls_ok || !tls_handshake)
		return;

	while (tlserr_again(r))
	{
		r = gnutls_record_send(tls_session, s.c_str(), s.size());
		if (r <= 0)
		{
			sock_ok = false;
			if (r == 0)
			{
				tls_ok = false;
				throw SockError("Connection reset by peer...");
			}
			else if (gnutls_error_is_fatal(r))
			{
				tls_err = r;
				CheckTLSError();
			}
			else
				usleep(100);
		}
	}
}

string SockWrapperTLS::GetClientUsername()
{
	const gnutls_datum_t *cert_list;
	unsigned int cert_list_size = 0;
	gnutls_x509_crt_t cert;
	unsigned int status;
	static char buf[4096];
	size_t size;
	string dn, username = "";

	if (!trust_check || !tls_ok)
		return "";

	b_log[W_INFO] << "Looking for Client Username";

	/* accept X.509 certificates only */
	if (gnutls_certificate_type_get(tls_session) != GNUTLS_CRT_X509)
	{
		b_log[W_INFO] << "TLS Client Certificate is not of X.509 type";
		return "";
	}

	cert_list = gnutls_certificate_get_peers(tls_session, &cert_list_size);
	if (cert_list_size <= 0)
	{
		b_log[W_INFO] << "No TLS Client Certificate provided";
		return "";
	}

	tls_err = gnutls_certificate_verify_peers2(tls_session, &status);
	CheckTLSError();
	if (status != 0)
	{
		b_log[W_INFO] << "TLS Client Certificate not valid";
		return "";
	}

	tls_err = gnutls_x509_crt_init(&cert);
	CheckTLSError();

	tls_err = gnutls_x509_crt_import(cert, &cert_list[0], GNUTLS_X509_FMT_DER);
	if (tls_err == GNUTLS_E_SUCCESS)
	{
		size = sizeof(buf);
		tls_err = gnutls_x509_crt_get_dn(cert, buf, &size);
		dn = buf;

		size_t pos1, pos2;
		pos1 = dn.find("CN=");
		if (pos1 != string::npos)
		{
			pos2 = dn.find(",", pos1);
			if (pos2 != string::npos)
				username = dn.substr(pos1 + 3, pos2 - pos1 - 3);
		}
	}
	gnutls_x509_crt_deinit(cert);
	CheckTLSError();

	if (username.empty())
		b_log[W_INFO] << "Client Username not found";
	else
		b_log[W_INFO] << "Client Username: " << username;
	return username;
}

};

