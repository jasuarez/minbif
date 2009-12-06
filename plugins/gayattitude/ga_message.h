#ifndef GA_GAMESSAGE_H
#define GA_GAMESSAGE_H

#include <purple.h>
#include "ga_account.h"
#include "ga_buddy.h"


typedef struct _GayAttitudeDelayedMessageRequest GayAttitudeDelayedMessageRequest;
typedef struct _GayAttitudeConversationInfo GayAttitudeConversationInfo;
typedef struct _GayAttitudeMessageExtraInfo GayAttitudeMessageExtraInfo;

struct _GayAttitudeDelayedMessageRequest {
	GayAttitudeAccount	*gaa;
	GayAttitudeBuddy	*gabuddy;
	gchar			*what;
	PurpleMessageFlags	flags;
};

struct _GayAttitudeConversationInfo {
	GayAttitudeBuddy	*gabuddy;
	guint64			latest_msg_id;
	gchar			*url_path, *checksum;
	time_t			timestamp;
	gboolean		replied;
};

struct _GayAttitudeMessageExtraInfo {
	guint64			id;
	gchar			*sender, *conv_name, *msg;
};

int ga_message_send(GayAttitudeAccount *gaa, GayAttitudeBuddy *gabuddy, const char *what, PurpleMessageFlags flags);
void ga_message_check_received(GayAttitudeAccount *gaa);
void ga_message_close_conversation(GayAttitudeAccount *gaa, const gchar *conv_name);

#endif /* GA_GAMESSAGE_H */
