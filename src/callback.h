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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <libpurple/purple.h>

class _CallBack
{
public:
	virtual ~_CallBack() {}
	virtual bool run() = 0;
};

template<typename T>
class CallBack : public _CallBack
{
public:

        typedef bool (T::*TFunc) (void*);

        CallBack(T* _obj, TFunc _func, void* _data = 0) : obj(_obj), func(_func), data(_data) {}

        virtual bool run()
        {
                return (obj->*func) (data);
        }

private:
        T* obj;
        TFunc func;
        void *data;
};

gboolean g_callback(void* data);
void g_callback_input(void* data, gint n = 0, PurpleInputCondition input = (PurpleInputCondition)0);

#endif /* CALLBACK_H */
