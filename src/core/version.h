/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009-2010 Romain Bignon
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
#define MINBIF_WEBSITE      "http://minbif.im"
#define MINBIF_DEV_WEBSITE  "http://symlink.me/projects/show/minbif"

#define MINBIF_VERSION_MAJOR   "1"
#define MINBIF_VERSION_MINOR   "0.5"
#define MINBIF_VERSION_PATCH   ""
#define MINBIF_VERSION_EXTRA   ""

#ifdef HAVE_CACA
#define MINBIF_SUPPORTS_CACA "(caca)"
#else
#define MINBIF_SUPPORTS_CACA ""
#endif

#ifdef HAVE_VIDEO
#define MINBIF_SUPPORTS_VIDEO "(video)"
#else
#define MINBIF_SUPPORTS_VIDEO ""
#endif

#ifdef HAVE_PAM
#define MINBIF_SUPPORTS_PAM "(pam)"
#else
#define MINBIF_SUPPORTS_PAM ""
#endif

#ifdef HAVE_TLS
#define MINBIF_SUPPORTS_TLS "(tls)"
#else
#define MINBIF_SUPPORTS_TLS ""
#endif

#define MINBIF_SUPPORTS      MINBIF_SUPPORTS_CACA \
	                     MINBIF_SUPPORTS_VIDEO \
                             MINBIF_SUPPORTS_PAM \
                             MINBIF_SUPPORTS_TLS

#define MINBIF_BUILD         ("(Build at " __DATE__ " " __TIME__ " with libpurple-" \
                              + t2s(PURPLE_MAJOR_VERSION) + "." \
                              + t2s(PURPLE_MINOR_VERSION) + "." \
                              + t2s(PURPLE_MICRO_VERSION) + ")")

#define MINBIF_VERSION       MINBIF_VERSION_NAME "-" \
			     MINBIF_VERSION_MAJOR "." \
			     MINBIF_VERSION_MINOR \
			     MINBIF_VERSION_PATCH \
			     MINBIF_VERSION_EXTRA \
			     MINBIF_SUPPORTS

/** NULL terminated array. */
extern const char* infotxt[];

#endif /* VERSION_H */
