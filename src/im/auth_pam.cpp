/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2010 Romain Bignon, Marc Dequènes (Duck)
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
#include "auth.h"
#include "core/log.h"
#include "core/util.h"
#include "core/config.h"
#include "irc/irc.h"
#include "auth_pam.h"

namespace im
{
AuthPAM::AuthPAM(irc::IRC* _irc, const string& _username)
	: Auth(_irc, _username)
{
	pamh = NULL;
}

AuthPAM::~AuthPAM()
{
	close();
}

bool AuthPAM::exists()
{
	return true;
}

static int pam_conv_func(int num_msg, const struct pam_message **msgm, struct pam_response **response, void *appdata_ptr)
{
	struct _pam_conv_func_data *func_data = (_pam_conv_func_data*) appdata_ptr;

	int count = 0;
	struct pam_response *reply;
	const char *reply_msg;

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
				/* sometimes the current password is asked first, sometimes not, how are we supposed to know ??? */
				if (func_data->update && strstr(msgm[count]->msg, " new "))
					reply_msg = (const char*) func_data->new_password.c_str();
				else
					reply_msg = (const char*) func_data->password.c_str();
				reply[count].resp = strndup(reply_msg, PAM_MAX_MSG_SIZE);

				b_log[W_DEBUG] << "PAM: msg " << count << ": " << msgm[count]->msg;
				b_log[W_DEBUG] << "PAM: msg " << count << ": " << reply_msg;
				break;
			case PAM_ERROR_MSG:
				b_log[W_ERR] << "PAM: " << msgm[count]->msg;
				break;
			case PAM_TEXT_INFO:
				b_log[W_DEBUG] << "PAM: " << msgm[count]->msg;
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

bool AuthPAM::checkPassword(const string& password)
{
	int retval;

	if (pamh)
		close();

	pam_conversation.conv = pam_conv_func;
	pam_conversation.appdata_ptr = (void*) &pam_conv_func_data;

	pam_conv_func_data.update = false;
	pam_conv_func_data.password = password;

	retval = pam_start("minbif", username.c_str(), &pam_conversation, &pamh);
	if (retval == PAM_SUCCESS)
		retval = pam_authenticate(pamh, 0);	/* is user really user? */

	if (retval == PAM_SUCCESS)
		retval = pam_acct_mgmt(pamh, 0);	/* permitted access? */

	if (retval != PAM_SUCCESS)
	{
		close(retval);

		return false;
	}

	return true;

}

bool AuthPAM::authenticate(const string& password)
{
	b_log[W_DEBUG] << "Authenticating user " << username << " using PAM mechanism";

	if(checkPassword(password))
	{
		im = new im::IM(irc, username);
		return true;
	}

	return false;
}

void AuthPAM::close(int retval)
{
	int retval2;

	if (!pamh)
		return;

	retval2 = pam_end(pamh, retval);
	if (retval2 != PAM_SUCCESS)       /* close Linux-PAM */
		throw IMError(string("PAM: Could not release authenticator: ") + pam_strerror(pamh, retval2));
	pamh = NULL;
}

bool AuthPAM::setPassword(const string& password)
{
	int retval;

	/* It should never happen */
	if (!pamh)
		throw IMError("Cannot change password if not authenticated");

	pam_conv_func_data.update = true;
	pam_conv_func_data.new_password = password;

	retval = pam_chauthtok(pamh, NULL);
	if (retval != PAM_SUCCESS)
		b_log[W_ERR] << "PAM: Password change failed: " << pam_strerror(pamh, retval);

	return (retval == PAM_SUCCESS);
}

string AuthPAM::getPassword() const
{
	b_log[W_WARNING] << "Cannot fetch current password, it is hidden";
	return "";
}

im::IM* AuthPAM::create(const string& password)
{
	if(!checkPassword(password))
		throw IMError("PAM: Incorrect login/password");

	return Auth::create(password);
}

}; /* namespace im */
