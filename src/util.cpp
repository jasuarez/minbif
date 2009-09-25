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

#include <string>
#include <cstdarg>

#include "util.h"

string stringtok(string &in, const char * const delimiters)
{
	string::size_type i = 0;
	string s;

	// eat leading whitespace
	i = in.find_first_not_of (delimiters, i);

	// find the end of the token
	string::size_type j = in.find_first_of (delimiters, i);

	if (j == string::npos)
	{
		if(i == string::npos)
			s = "";
		else
			s = in.substr(i);
		in = "";
		return s;			  // nothing left but white space
	}

	// push token
	s = in.substr(i, j-i);
	in = in.substr(j+1);

	return s;
}

typedef struct _PurpleGLibIOClosure {
	PurpleInputFunction function;
	guint result;
	gpointer data;
} PurpleGLibIOClosure;

#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

static void purple_glib_io_destroy(gpointer data)
{
        g_free(data);
}

static gboolean purple_glib_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
        PurpleGLibIOClosure *closure = (PurpleGLibIOClosure*)data;
        PurpleInputCondition purple_cond = (PurpleInputCondition)0;

        if (condition & PURPLE_GLIB_READ_COND)
                purple_cond = (PurpleInputCondition)(purple_cond|PURPLE_INPUT_READ);
        if (condition & PURPLE_GLIB_WRITE_COND)
                purple_cond = (PurpleInputCondition)(purple_cond|PURPLE_INPUT_WRITE);

        closure->function(closure->data, g_io_channel_unix_get_fd(source),
                          purple_cond);

        return TRUE;
}

guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function,
                                                           gpointer data)
{
        PurpleGLibIOClosure *closure = g_new0(PurpleGLibIOClosure, 1);
        GIOChannel *channel;
        GIOCondition cond = (GIOCondition)0;

        closure->function = function;
        closure->data = data;

        if (condition & PURPLE_INPUT_READ)
                cond = (GIOCondition)(cond|PURPLE_GLIB_READ_COND);
        if (condition & PURPLE_INPUT_WRITE)
                cond = (GIOCondition)(cond|PURPLE_GLIB_WRITE_COND);

        channel = g_io_channel_unix_new(fd);
        closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
                                              purple_glib_io_invoke, closure, purple_glib_io_destroy);

        g_io_channel_unref(channel);
        return closure->result;
}


string strupper(string s)
{
	for(string::iterator it = s.begin(); it != s.end(); ++it)
		*it = (char)toupper(*it);
	return s;
}

string strlower(string s)
{
	for(string::iterator it = s.begin(); it != s.end(); ++it)
		*it = (char)tolower(*it);
	return s;
}

bool is_ip(const char *ip)
{
	char *ptr = NULL;
	int i = 0, d = 0;

	for(; i < 4; ++i)			  /* 4 dots expected (IPv4) */
	{					  /* Note about strtol: stores in endptr either NULL or '\0' if conversion is complete */
		if(!isdigit((unsigned char) *ip)  /* most current case (not ip, letter host) */
						  /* ok, valid number? */
			|| (d = (int)strtol(ip, &ptr, 10)) < 0 || d > 255
			|| (ptr && *ptr != 0 && (*ptr != '.' || 3 == i) && ptr != ip)) return false;
		if(ptr) ip = ptr + 1, ptr = NULL;  /* jump the dot */
	}
	return true;
}


