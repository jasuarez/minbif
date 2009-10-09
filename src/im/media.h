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

#ifndef IM_MEDIA_H
#define IM_MEDIA_H

#include <libpurple/purple.h>
#include <vector>
#include <string>

#include "mutex.h"

#ifdef HAVE_VIDEO
#include <libpurple/media-gst.h>
#include "buddy.h"
#include "caca_image.h"
#endif

class _CallBack;

namespace irc {
	class DCCChat;
};

namespace im {

	using std::vector;
	using std::string;

#ifdef HAVE_VIDEO
	class Media;
	class MediaList : public Mutex
	{
		vector<Media> medias;
		_CallBack* check_cb;
		int check_id;
	public:

		MediaList();
		~MediaList();

		void addMedia(const Media& media);
		void removeMedia(const Media& media);
		Media getMedia(PurpleMedia* m);
		void enqueueBuffer(const Media& media, const CacaImage& buf);

		bool check(void*);
	};
#endif /* HAVE_VIDEO */

	class Media
	{
#ifdef HAVE_VIDEO
		PurpleMedia* media;
		Buddy buddy;
		vector<CacaImage> queue;
		irc::DCCChat* dcc;

		static MediaList media_list;
		static bool gstreamer_init_failed;

		static GstElement *create_default_video_src(PurpleMedia *media,
					const gchar *session_id, const gchar *participant);
		static GstElement *create_default_video_sink(PurpleMedia *media,
					const gchar *session_id, const gchar *participant);
		static gboolean media_new_cb(PurpleMediaManager *manager, PurpleMedia *media,
				PurpleAccount *account, gchar *screenname, gpointer nul);
		static gboolean minbif_media_ready_cb(PurpleMedia *media);

		static void minbif_media_state_changed_cb(PurpleMedia *media, PurpleMediaState state,
				gchar *sid, gchar *name, void* gtkmedia);
		static void got_data(GstElement* object,
				GstBuffer* buffer,
				GstPad* arg1,
				gpointer user_data);
#endif /* HAVE_VIDEO */

	public:

		static void init();
		static void uninit();

#ifdef HAVE_VIDEO
		Media();
		Media(PurpleMedia*);
		Media(PurpleMedia*, const Buddy& b);
		Media(const Media&);
		~Media();

		Media& operator=(const Media&);
		bool operator==(const Media&) const;
		bool operator!=(const Media&) const;

		bool isValid() const { return media; }

		void enqueueBuffer(const CacaImage& buf);
		void checkBuffer();
		Buddy getBuddy() const { return buddy; }
		PurpleMedia* getPurpleMedia() const { return media; }
#endif /* HAVE_VIDEO */
	};

}; /* ns im */

#endif /* IM_MEDIA_H */
