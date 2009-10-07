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

#ifndef SIGHANDLER_H
#define SIGHANDLER_H

class Minbif;

class SigHandler
{
	Minbif* app;

	static void handler(int r);
	bool rehash(void*);
	bool quit(void*);

public:

	SigHandler();
	~SigHandler();

	void setApplication(Minbif* app);
};

extern SigHandler sighandler;

#endif /* SIGHANDLER_H */
