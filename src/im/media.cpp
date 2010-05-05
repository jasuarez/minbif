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

#include <cstring>
#include "im/media.h"
#include "im/im.h"
#include "im/purple.h"
#include "irc/irc.h"
#include "irc/dcc.h"
#include "irc/user.h"
#include "irc/buddy.h"
#include "core/log.h"
#include "core/caca_image.h"
#include "core/util.h"
#include "core/callback.h"

namespace im {

#ifdef HAVE_VIDEO

MediaList::MediaList()
	: Mutex(),
	  check_cb(NULL),
	  check_id(-1)
{
}

MediaList::~MediaList()
{
	if(check_id >= 0)
		g_source_remove(check_id);
	delete check_cb;
}

void MediaList::addMedia(const Media& media)
{
	BlockLockMutex m(this);
	if(check_id < 0)
	{
		check_cb = new CallBack<MediaList>(this, &MediaList::check);
		check_id = g_timeout_add(50, g_callback, check_cb);
	}
	medias.push_back(media);
}

Media MediaList::getMedia(PurpleMedia* m)
{
	BlockLockMutex lock(this);
	vector<Media>::iterator it;
	for(it = medias.begin(); it != medias.end() && it->getPurpleMedia() != m; ++it)
		;
	if(it == medias.end())
		return Media();
	else
		return *it;
}

void MediaList::removeMedia(const Media& media)
{
	BlockLockMutex m(this);
	vector<Media>::iterator it;
	for(it = medias.begin(); it != medias.end(); )
		if(*it == media)
			it = medias.erase(it);
		else
			++it;
}

void MediaList::enqueueBuffer(const Media& media, const CacaImage& buf)
{
	BlockLockMutex m(this);
	vector<Media>::iterator it;
	for(it = medias.begin(); it != medias.end() && *it != media; ++it)
		;

	if(it == medias.end())
	{
		b_log[W_WARNING] << "Trying to enqueue data to an unknown stream.";
		return;
	}

	it->enqueueBuffer(buf);
}

bool MediaList::check(void*)
{
	BlockLockMutex m(this);
	vector<Media>::iterator it;
	for(it = medias.begin(); it != medias.end(); ++it)
		it->checkBuffer();

	return true;
}

Media::Media()
	: media(0),
	  dcc(NULL)
{
}

Media::Media(PurpleMedia* m)
	: media(m),
	  dcc(NULL)
{}

Media::Media(PurpleMedia* m, const Buddy& b)
	: media(m),
	  buddy(b),
	  dcc(NULL)
{
}

Media::Media(const Media& m)
	: media(m.media),
	  buddy(m.buddy),
	  dcc(NULL)
{

}

Media& Media::operator=(const Media& m)
{
	media = m.media;
	buddy = m.buddy;
	delete dcc;
	dcc = NULL;
	return *this;
}

Media::~Media()
{
	delete dcc;
}

bool Media::operator==(const Media& m) const
{
	return this->media && this->media == m.media;
}

bool Media::operator!=(const Media& m) const
{
	return !this->media || this->media != m.media;
}

void Media::enqueueBuffer(const CacaImage& buf)
{
	queue.push_back(buf);
}

void Media::checkBuffer()
{
	vector<CacaImage>::iterator it;
	for(it = queue.begin(); it != queue.end(); it = queue.erase(it))
	{
		try
		{
			if(!dcc)
			{
				irc::IRC* irc = Purple::getIM()->getIRC();
				irc::Buddy* sender = irc->getNick(buddy);
				dcc = new irc::DCCChat(sender, irc->getUser());
			}
			string buf = it->getIRCBuffer(0, 20, "ansi"), line;
			dcc->dcc_send(buf);
		}
		catch(CacaError &e)
		{
			b_log[W_ERR] << "Caca error while sending to user";
		}
	}
}

#endif /* HAVE_VIDEO */

/* STATIC */

void Media::init()
{
#ifdef HAVE_VIDEO
	GError *error = NULL;
	if((gstreamer_init_failed = !gst_init_check(NULL, NULL, &error)))
	{
		b_log[W_ERR] << "Unable to initialize GStreamer: " << (error ? error->message : "");
		if(error)
			g_free(error);
		return;
	}

	PurpleMediaManager *manager = purple_media_manager_get();
	PurpleMediaElementInfo *default_video_src =
			(PurpleMediaElementInfo*)g_object_new(PURPLE_TYPE_MEDIA_ELEMENT_INFO,
			"id", "minbifdefaultvideosrc",
			"name", "minbif Default Video Source",
			"type", PURPLE_MEDIA_ELEMENT_VIDEO
					| PURPLE_MEDIA_ELEMENT_SRC
					| PURPLE_MEDIA_ELEMENT_ONE_SRC
					| PURPLE_MEDIA_ELEMENT_UNIQUE,
			"create-cb", create_default_video_src, NULL);
	PurpleMediaElementInfo *default_video_sink =
			(PurpleMediaElementInfo*)g_object_new(PURPLE_TYPE_MEDIA_ELEMENT_INFO,
			"id", "minbifdefaultvideosink",
			"name", "minbif Default Video Sink",
			"type", PURPLE_MEDIA_ELEMENT_VIDEO
					| PURPLE_MEDIA_ELEMENT_SINK
					| PURPLE_MEDIA_ELEMENT_ONE_SINK,
			"create-cb", create_default_video_sink, NULL);

	g_signal_connect(G_OBJECT(manager), "init-media",
			 G_CALLBACK(media_new_cb), NULL);

	purple_media_manager_set_ui_caps(manager, (PurpleMediaCaps)(
			PURPLE_MEDIA_CAPS_VIDEO |
			PURPLE_MEDIA_CAPS_VIDEO_SINGLE_DIRECTION));

	purple_media_manager_set_active_element(manager, default_video_sink);
	purple_media_manager_set_active_element(manager, default_video_src);

#endif /* HAVE_VIDEO */
}

void Media::uninit()
{
#ifdef HAVE_VIDEO
	if(!gstreamer_init_failed)
		gst_deinit();
#endif /* HAVE_VIDEO */
}

#ifdef HAVE_VIDEO
MediaList Media::media_list;
bool Media::gstreamer_init_failed = true;

static void
minbif_media_accept_cb(PurpleMedia *media, int index)
{
	purple_media_stream_info(media, PURPLE_MEDIA_INFO_ACCEPT,
			NULL, NULL, TRUE);
}

static void
minbif_media_reject_cb(PurpleMedia *media, int index)
{
	purple_media_stream_info(media, PURPLE_MEDIA_INFO_REJECT,
			NULL, NULL, TRUE);
}


gboolean Media::minbif_media_ready_cb(PurpleMedia *media)
{
	Media m = media_list.getMedia(media);
	string alias = m.getBuddy().getAlias();

	//PurpleMediaSessionType type = purple_media_get_session_type(media, sid);
	PurpleMediaSessionType type = PURPLE_MEDIA_VIDEO;
	gchar *message = NULL;

	PurpleAccount* account = purple_media_get_account(media);

	if (type & PURPLE_MEDIA_AUDIO && type & PURPLE_MEDIA_VIDEO) {
		message = g_strdup_printf("%s wishes to start an audio/video session with you.",
				alias.c_str());
	} else if (type & PURPLE_MEDIA_AUDIO) {
		message = g_strdup_printf("%s wishes to start an audio session with you.",
				alias.c_str());
	} else if (type & PURPLE_MEDIA_VIDEO) {
		message = g_strdup_printf("%s wishes to start a video session with you.",
				alias.c_str());
	}

/* purple_request_accept_cancel is a macro and calls _() to translates
 * buttons strings.
 * There isn't (yet?) any translation system in Minbif, so the _ macro
 * is defined to make minbif compiles.
 */
#define _
	purple_request_accept_cancel(media, "Incoming Call",
			message, NULL, PURPLE_DEFAULT_ACTION_NONE,
			account, alias.c_str(), NULL,
			media,
			minbif_media_accept_cb,
			minbif_media_reject_cb);
#undef _
	g_free(message);
	return FALSE;
}

static void
minbif_media_error_cb(PurpleMedia *media, const char *error, void *gtkmedia)
{
	b_log[W_ERR] << "Error: " << error;
}

void Media::minbif_media_state_changed_cb(PurpleMedia *media, PurpleMediaState state,
		gchar *sid, gchar *name, void* gtkmedia)
{
	if (sid == NULL && name == NULL) {
		if (state == PURPLE_MEDIA_STATE_END) {
			media_list.removeMedia(Media(media));
			b_log[W_INFO] << "The call has been terminated.";
		}
	} else if (state == PURPLE_MEDIA_STATE_NEW &&
			sid != NULL && name != NULL) {
		if (purple_media_is_initiator(media, sid, NULL) == FALSE) {
			g_timeout_add(500,
				(GSourceFunc)Media::minbif_media_ready_cb,
				gtkmedia);
			purple_media_set_output_window(media, sid,
					name, 0);
		}
	}
}

static void
minbif_media_stream_info_cb(PurpleMedia *media, PurpleMediaInfoType type,
		gchar *sid, gchar *name, gboolean local,
		void *gtkmedia)
{
	if (type == PURPLE_MEDIA_INFO_REJECT) {
		b_log[W_INFO] << "You have rejected the call.";
	} else if (type == PURPLE_MEDIA_INFO_ACCEPT) {
		b_log[W_INFO] << "Call in progress";
	}
}

gboolean Media::media_new_cb(PurpleMediaManager *manager, PurpleMedia *media,
		PurpleAccount *account, gchar *screenname, gpointer nul)
{
	PurpleBuddy *buddy = purple_find_buddy(account, screenname);
	const gchar *alias = buddy ?
			purple_buddy_get_contact_alias(buddy) : screenname;
	b_log[W_INFO] << "Started video with " << alias;
	media_list.addMedia(Media(media, Buddy(buddy)));

	g_signal_connect(G_OBJECT(media), "error",
		G_CALLBACK(minbif_media_error_cb), media);
	g_signal_connect(G_OBJECT(media), "state-changed",
		G_CALLBACK(minbif_media_state_changed_cb), media);
	g_signal_connect(G_OBJECT(media), "stream-info",
		G_CALLBACK(minbif_media_stream_info_cb), media);

	return TRUE;

}

GstElement *Media::create_default_video_src(PurpleMedia *media,
			const gchar *session_id, const gchar *participant)
{
	GstElement *sendbin, *src, *videoscale, *capsfilter;
	GstPad *pad;
	GstPad *ghost;
	GstCaps *caps;

	src = gst_element_factory_make("videotestsrc", NULL);
	if (src == NULL) {
		purple_debug_error("gtkmedia", "Unable to find a suitable "
				"element for the default video source.\n");
		return NULL;
	}
	g_object_set (G_OBJECT (src), "is-live", true, NULL);

	sendbin = gst_bin_new("minbifdefaultvideosrc");
	videoscale = gst_element_factory_make("videoscale", NULL);
	capsfilter = gst_element_factory_make("capsfilter", NULL);

	/* It was recommended to set the size <= 352x288 and framerate <= 20 */
	caps = gst_caps_from_string("video/x-raw-yuv , width=[250,352] , "
			"height=[200,288] , framerate=[1/1,20/1]");
	g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);

	gst_bin_add_many(GST_BIN(sendbin), src,
			videoscale, capsfilter, NULL);
	gst_element_link_many(src, videoscale, capsfilter, NULL);

	pad = gst_element_get_static_pad(capsfilter, "src");
	ghost = gst_ghost_pad_new("ghostsrc", pad);
	gst_object_unref(pad);
	gst_element_add_pad(sendbin, ghost);

	return sendbin;
}

void Media::got_data(GstElement* object,
		GstBuffer* buffer,
		GstPad* arg1,
		gpointer user_data)
{
	try
	{
		gint w, h;
		guint bpp = 0;
		GstStructure* structure = gst_caps_get_structure (buffer->caps, 0);
		gst_structure_get_int (structure, "width", &w);
		gst_structure_get_int (structure, "height", &h);
		gst_structure_get_int (structure, "bpp", (int *) &bpp);
		if(!bpp)
			bpp = 8;

		CacaImage img(GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer), w, h, bpp);
		Media::media_list.enqueueBuffer(Media((PurpleMedia*)user_data), img);
	}
	catch(CacaError& e)
	{
	}
}

GstElement *Media::create_default_video_sink(PurpleMedia *media,
			const gchar *session_id, const gchar *participant)
{
	GstElement *ffmpegcolorspace, *fakesink;

	ffmpegcolorspace = gst_element_factory_make("ffmpegcolorspace", NULL);
	fakesink = gst_element_factory_make("fakesink", NULL);
	g_object_set(G_OBJECT(fakesink), "signal-handoffs", TRUE, NULL);
	g_signal_connect(fakesink, "handoff", G_CALLBACK(got_data), media);

	gst_element_link_many(ffmpegcolorspace, fakesink, NULL);

	return fakesink;
}

#endif /* HAVE_VIDEO */

} /* ns im */
