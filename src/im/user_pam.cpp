/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2010 Marc Dequènes (Duck)
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
#include "user_pam.h"

namespace im
{
UserPAM::UserPAM(irc::IRC* _irc, string _username)
	: User(_irc, _username)
{
}

bool UserPAM::exists()
{
	return true;
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

bool UserPAM::authenticate(const string password)
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

	if (retval == PAM_SUCCESS)
		im = new im::IM(irc, username);

	return (retval == PAM_SUCCESS);
}
}; /* namespace im */
