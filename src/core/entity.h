/*
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

#ifndef ENTITY_H
#define ENTITY_H

#include <string>
using std::string;

class Entity
{
	string name;

public:

	Entity(string _name)
		: name(_name)
	{}
	virtual ~Entity() {}

	string getName() const { return name; }
	void setName(string n) { name = n; }

	virtual string getLongName() const { return name; }


};

#endif /* ENTITY_H */
