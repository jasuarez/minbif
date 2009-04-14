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

#ifndef IM_PURPLE_HANDLER_H
#define IM_PURPLE_HANDLER_H

/********************************
 * XXX XXX XXX XXX XXX XXX XXX
 * XXX  D I S C L A M E R  XXX
 * XXX XXX XXX XXX XXX XXX XXX
 *
 * This  class is  a ugly  hack to
 * bypass problem with C callbacks
 * on  libpurple.
 *
 ********************************/

class IM;

class PurpleHandler
{
	/* Instanciation is forbidden */
	PurpleHandler() {}
	~PurpleHandler() {}

	static IM*
};

#endif /* IM_PURPLE_HANDLER_H */
