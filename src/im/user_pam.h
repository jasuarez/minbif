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

#ifndef IM_USER_PAM_H
#define IM_USER_PAM_H

#include "user.h"
#include <security/pam_appl.h>
#include <security/pam_misc.h>

/** IM related classes */
namespace im
{
	class UserPAM : public User
	{
	public:
		UserPAM(irc::IRC* _irc, string _username);
		bool exists();
		bool authenticate(const string password);

	private:
		struct pam_conv pam_conversation;
	};
};

#endif /* IM_USER_PAM_H */
