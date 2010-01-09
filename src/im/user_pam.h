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

#ifndef IM_USER_PAM_H
#define IM_USER_PAM_H

#include "user.h"
#include <security/pam_appl.h>
#include <security/pam_misc.h>

struct _pam_conv_func_data {
	bool update;
	string password;
	string new_password;
};

/** IM related classes */
namespace im
{
	using std::string;

	class UserPAM : public User
	{
	public:
		UserPAM(irc::IRC* _irc, string _username);
		~UserPAM();
		bool exists();
		bool authenticate(const string password);
		bool setPassword(const string& password);
		string getPassword() const;

	private:
		pam_handle_t *pamh;
		struct pam_conv pam_conversation;
		struct _pam_conv_func_data pam_conv_func_data;

		void close(int retval = PAM_SUCCESS);
	};
};

#endif /* IM_USER_PAM_H */
