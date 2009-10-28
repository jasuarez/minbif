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

#include <glib/gstdio.h>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
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

bool check_write_file(string path, string filename)
{
	/* Create the directory if not exist. */
	DIR* d;
	if(!(d = opendir(path.c_str())))
	{
		if(mkdir(path.c_str(), 0700) < 0)
			return false;
	}
	else
		closedir(d);

	path += "/" + filename;

	struct stat st;
	if (g_stat(path.c_str(), &st) != 0)
	{
		/* File doesn't exist. */
		const char* dir = g_path_get_dirname(path.c_str());

		if(g_access(dir, W_OK) != 0)
			return false;
	}
	else if(S_ISDIR(st.st_mode))
		return false;

	return true;
}

gchar* markup2irc(const gchar* markup)
{
	char* newline = purple_strdup_withhtml(markup);
	GString* s = g_string_sized_new(strlen(newline));
	gchar *ptr, *next, *buf;

	for(ptr = newline; *ptr; ptr = next)
	{
		next = g_utf8_next_char(ptr);
		if(*ptr == '<')
		{
			bool closed = false;
			++ptr;
			if(*ptr == '/')
			{
				closed = true;
				++ptr;
			}
			while(*next && *next != '>')
				next = g_utf8_next_char(next);
			if(*next)
				next = g_utf8_next_char(next);

			switch(*ptr)
			{
				case 'i':
				case 'I':
					g_string_append_c(s, '\011');
					break;
				case 'u':
				case 'U':
					g_string_append_c(s, '\037');
					break;
				case 'B':
				case 'b':
					g_string_append_c(s, '\002');
					break;
				case 'F':
				case 'f':
					if(closed && !strncasecmp(ptr, "FONT", 4))
					{
						g_string_append_c(s, '\003');
						break;
					}
					else if(!closed && !strncasecmp(ptr, "FONT ", 5))
					{
						static struct
						{
							const char* num;
							const char* name;
						} irc_colors[] = {
							{ "00",  "white"      },
							{ "01",  "black"      },
							{ "02",  "blue"       },
							{ "03",  "dark green" },
							{ "04",  "red"        },
							{ "05",  "brown"      },
							{ "06",  "purple"     },
							{ "07",  "orange"     },
							{ "08",  "yellow"     },
							{ "09",  "green"      },
							{ "10", "teal"       },
							{ "11", "cyan"       },
							{ "12", "light blue" },
							{ "13", "pink"       },
							{ "14", "grey"       },
							{ "15", "light grey" },
						};
						const char *foreground = NULL, *background = NULL;
						bool is_foreground;
						gchar* space;

						ptr += 5;
						/* loop on all attributes of the FONT tag to get
						 * IRC colors.
						 */
						while(*ptr && *ptr != '>')
						{
							unsigned i;
							if(!strncasecmp(ptr, "color=", 6))
							{
								ptr += 6;
								if(*ptr == '"') ++ptr;
								is_foreground = true;
							}
							else if(!strncasecmp(ptr, "back=", 5))
							{
								ptr += 5;
								if(*ptr == '"') ++ptr;
								is_foreground = false;
							}
							else
								break;

							space = ptr;
							while(*space && *space != '"')
								space = g_utf8_next_char(space);
							for(i = 0; i < sizeof irc_colors / sizeof *irc_colors; ++i)
								if(!strncasecmp(ptr, irc_colors[i].name, space-ptr))
								{
									if(is_foreground)
										foreground = irc_colors[i].num;
									else
										background = irc_colors[i].num;
									break;
								}

							if(*space == '"')
								++space;

							ptr = space;
							while(*ptr && *ptr == ' ')
								ptr = g_utf8_next_char(ptr);
						}

						if(foreground)
						{
							g_string_append_c(s, '\003');
							g_string_append(s, foreground);
							if(background)
							{
								g_string_append_c(s, ',');
								g_string_append(s, background);
							}
						}
						break;
					}
				default:
					g_string_append_c(s, '<');
					if(closed)
						g_string_append_c(s, '/');
					g_string_append_len(s, ptr, next-ptr);
					continue;
			}
		}
		else
			g_string_append_len(s, ptr, next-ptr);
	}
	g_free(newline);

	/* Strip all other html tags */
	newline = g_string_free(s, FALSE);
	buf = purple_markup_strip_html(newline);

	g_free(newline);
	return buf;
}


