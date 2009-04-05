/*
 * Copyright(C) 2008-2009 Laurent Defert, Romain Bignon
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

#ifndef LOG_H
#define LOG_H

#include <string>
#include <sstream>

enum
{
	W_DEBUG      = 1 << 0,			  /* Debug */
	W_PARSE      = 1 << 1,			  /* Show parsing */
	W_ROUTING    = 1 << 2,                    /* Routing information */
	W_DESYNCH    = 1 << 3,			  /* Desynchronization */
	W_WARNING    = 1 << 4,			  /* Warnings */
	W_ERR        = 1 << 5,			  /* Errors */
	W_INFO       = 1 << 6			  /* Info */
};

#define DEFAULT_LOGGED_FLAGS (W_DESYNCH|W_WARNING|W_ERR|W_INFO)

#define FLog(flags, msg) b_log[flags] << __FILE__ << ":" << __PRETTY_FUNCTION << "():" << __LINE__ << ": " << msg

/***************************
 *     Log                 *
 ***************************
 *
 * Usage:
 *   b_log[flags] << messages;
 *
 * Examples:
 *   b_log[W_WARNING] << cl << "There is a problem with this: " << i;
 *   b_log[W_ERR] << "This user isn't allowed to do this!";
 */

class IRC;

class Log
{
public:
	Log();

	~Log();

	void SetLoggedFlags(std::string s, bool to_syslog = true);
	uint32_t LoggedFlags() const { return logged_flags; }
	bool ToSyslog() const { return to_syslog; }

	void setIRC(const IRC* _irc) { irc = _irc; }
	const IRC* getIRC() const { return irc; }

	class flux
	{
		std::string str;
		size_t flag;

		public:
			flux(size_t i)
				: flag(i)
				{}

			~flux();

			template<typename T>
				flux& operator<< (T s)
			{
				std::ostringstream oss;
				oss << s;
				str += oss.str();
				return *this;
			}
	};

	flux operator[](size_t __n)
	{
		return flux(__n);
	}

private:

	uint32_t logged_flags;
	bool to_syslog;
	const IRC* irc;
};

template<>
inline Log::flux& Log::flux::operator<< <std::string> (std::string s)
{
	str += s;
	return *this;
}

extern Log b_log;
#endif						  /* LOG_H */
