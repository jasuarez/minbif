/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009 Marc Dequènes, Romain Bignon
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

#ifndef GA_GAYATTITUDE_H
#define GA_GAYATTITUDE_H

#include <purple.h>

#include "../lib/http.h"
#include "ga_account.h"
#include "ga_buddy.h"
/* XXX: temporary until all processing stuff is split */
#include "ga_parsing.h"

#define GA_NAME "libpurple (gayattitude)"
#define GA_CHECK_INTERVAL 30

typedef struct _GayAttitudeDelayedMessageRequest GayAttitudeDelayedMessageRequest;

struct _GayAttitudeDelayedMessageRequest {
	GayAttitudeAccount	*gaa;
	GayAttitudeBuddy	*gabuddy;
	gchar			*what;
	PurpleMessageFlags	flags;
};

#endif /* GA_GAYATTITUDE_H */
