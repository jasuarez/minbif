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

#ifndef IM_CONVERSATION_H
#define IM_CONVERSATION_H

#include <libpurple/purple.h>
#include <string>

namespace im
{
	using std::string;

	class Conversation
	{
		PurpleConversation* buddy;

		static void* getHandler();
		static PurpleConversationUiOps conv_ui_ops;

		static void create(PurpleConversation*);
		static void write_im(PurpleConversation *conv, const char *who,
				const char *message, PurpleMessageFlags flags,
				time_t mtime);

	public:

		static void init();

		Conversation();
		Conversation(PurpleConversation* buddy);

		bool operator==(const Conversation& buddy) const;
		bool operator!=(const Conversation& buddy) const;

		bool isValid() const { return buddy != NULL; }

	};

};

#endif /* IM_CONVERSATION_H */
