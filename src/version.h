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

#ifndef VERSION_H
#define VERSION_H

#define MINBIF_VERSION_NAME "minbif"

#define MINBIF_VERSION_MAJOR   "1"
#define MINBIF_VERSION_MINOR   "0"
#define MINBIF_VERSION_PATCH   ""
#define MINBIF_VERSION_EXTRA   "dev"

#ifdef USE_CACA
#define MINBIF_SUPPORTS_CACA "-caca"
#else
#define MINBIF_SUPPORTS_CACA ""
#endif

#ifdef HAVE_VIDEO
#define MINBIF_SUPPORTS_VIDEO "-video"
#else
#define MINBIF_SUPPORTS_VIDEO ""
#endif

#define MINBIF_SUPPORTS      MINBIF_SUPPORTS_CACA \
	                     MINBIF_SUPPORTS_VIDEO

#define MINBIF_BUILD         "(Build " __DATE__ " " __TIME__ ")"

#define MINBIF_VERSION       MINBIF_VERSION_NAME \
			     MINBIF_VERSION_MAJOR "." \
			     MINBIF_VERSION_MINOR \
			     MINBIF_VERSION_PATCH \
			     MINBIF_VERSION_EXTRA \
			     MINBIF_SUPPORTS

#endif /* VERSION_H */
