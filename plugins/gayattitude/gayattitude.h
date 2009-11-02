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
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/HTMLparser.h>

#include "../lib/http.h"

#define GA_HOSTNAME "www.gayattitude.com"
#define GA_HOSTNAME_PERSO "perso.gayattitude.com"
#define GA_CHECK_INTERVAL 30

typedef struct _GayAttitudeAccount GayAttitudeAccount;
typedef struct _GayAttitudeBuddyInfoRequest GayAttitudeBuddyInfoRequest;

struct _GayAttitudeAccount {
	PurpleAccount *account;
	PurpleConnection *pc;
	HttpHandler* http_handler;
	guint new_messages_check_timer;
	GHashTable *ref_ids;
};

struct _GayAttitudeBuddyInfoRequest {
	gchar		*who;
	gboolean	advertise;
};

#endif /* GA_GAYATTITUDE_H */
