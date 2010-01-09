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

#include <cstring>
#include <cerrno>
#include "user.h"
#include "core/log.h"
#include "core/util.h"
#include "core/config.h"
#include "irc/irc.h"

namespace im
{
User::User(irc::IRC* _irc, string _username)
	: username(_username),
	  irc(_irc)
{
	use_pam = conf.GetSection("aaa")->GetItem("use_pam")->Boolean();
	im = NULL;
}

bool User::exists()
{
	if (use_pam)
		return true;
	else
		return im::IM::exists(username);
}

bool User::authenticate(const string password)
{
	if(use_pam)
	{
		if (!authenticate_pam(username, password))
			return false;
		im = new im::IM(irc, username);
	}
	else
	{
		if (!im::IM::exists(username))
			return false;

		im = new im::IM(irc, username);
		if (!authenticate_local(password))
			return false;
	}

	return true;
}

bool User::authenticate_local(const string password)
{
	b_log[W_INFO] << "Authenticating user " << im->getUsername() << " using local database";
	return im->getPassword() == password;
}

static int pam_conv_func(int num_msg, const struct pam_message **msgm, struct pam_response **response, void *appdata_ptr)
{
	int count = 0;
	struct pam_response *reply;

	reply = (struct pam_response *) calloc(num_msg, sizeof(struct pam_response));
	if (reply == NULL)
	{
		b_log[W_ERR] << "PAM: Could not allocate enough memory";
		return PAM_CONV_ERR;
	}

	for (count = 0; count < num_msg; ++count)
	{
		switch (msgm[count]->msg_style)
		{
			case PAM_PROMPT_ECHO_OFF:
			case PAM_PROMPT_ECHO_ON:
				reply[count].resp_retcode = 0;
				reply[count].resp = strndup((const char*)appdata_ptr, PAM_MAX_MSG_SIZE);
				break;
			case PAM_ERROR_MSG:
				b_log[W_ERR] << "PAM: " << msgm[count]->msg;
				break;
			case PAM_TEXT_INFO:
				b_log[W_INFO] << "PAM: " << msgm[count]->msg;
				break;
			default:
				b_log[W_ERR] << "PAM: erroneous conversation (" << msgm[count]->msg_style << ")";
				goto failed_conversation;
		}
	}

	*response = reply;

	return PAM_SUCCESS;

    failed_conversation:
	for (count = 0; count < num_msg; ++count)
	{
		if (reply[count].resp != NULL)
			free(reply[count].resp);
	}
	free(reply);

	*response = NULL;

	return PAM_CONV_ERR;
}

bool User::authenticate_pam(const string username, const string password)
{
	pam_handle_t *pamh=NULL;
	int retval, retval2;

	b_log[W_INFO] << "Authenticating user " << username << " using PAM mechanism";

	pam_conversation.conv = pam_conv_func;
	pam_conversation.appdata_ptr = (void*) password.c_str();

	retval = pam_start("minbif", username.c_str(), &pam_conversation, &pamh);
	if (retval == PAM_SUCCESS)
		retval = pam_authenticate(pamh, 0);	/* is user really user? */

	if (retval == PAM_SUCCESS)
		retval = pam_acct_mgmt(pamh, 0);	/* permitted access? */

	retval2 = pam_end(pamh, retval);
	if (retval2 != PAM_SUCCESS) {     /* close Linux-PAM */
		b_log[W_ERR] << "PAM: Could not release authenticatori: " << pam_strerror(pamh, retval2);
		throw IMError();
	}

	return (retval == PAM_SUCCESS);
}

im::IM* User::create(const string password)
{
	im = new im::IM(irc, username);
	im->setPassword(password);

	return im;
}

im::IM* User::getIM()
{
	return im;
}
}; /* namespace im */
