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

#ifndef PF_SOCKWRAP_PLAIN_H
#define PF_SOCKWRAP_PLAIN_H

class SockWrapperPlain : public SockWrapper
{
public:
	SockWrapperPlain(int _fd);
	~SockWrapperPlain();

	string Read();
	void Write(string s);
};

#endif /* PF_SOCKWRAP_PLAIN_H */
