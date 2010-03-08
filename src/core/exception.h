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

#ifndef _MINBIF_EXCEPTION_H
#define _MINBIF_EXCEPTION_H

#include <exception>
#include <string>

class StrException : public std::exception
{
	std::string reason;

public:
	StrException(const std::string& _reason) : reason(_reason) {}
	virtual ~StrException() throw() {}

	const std::string& Reason() const { return reason; }
};

#define EXCEPTION(x) class x : public std::exception {};
#define STREXCEPTION(x) class x : public StrException { public: x(const std::string& reason) : StrException(reason) {} };

#endif /* _MINBIF_EXCEPTION_H */
