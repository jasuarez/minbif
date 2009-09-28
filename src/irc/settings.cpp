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

#include "settings.h"
#include "im/im.h"

namespace irc
{

SettingBase::SettingBase(im::IM* _im)
	: im(_im)
{}

string SettingPassword::getValue() const
{
	return getIM()->getPassword();
}

bool SettingPassword::setValue(string v)
{
	if(v.find(' ') != string::npos || v.size() < 8)
		return false;

	getIM()->setPassword(v);
	return true;
}

}; /* ns irc */
