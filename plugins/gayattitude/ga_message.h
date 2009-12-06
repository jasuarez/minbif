#ifndef GA_GAMESSAGE_H
#define GA_GAMESSAGE_H

#include <purple.h>
#include "ga_account.h"
#include "ga_buddy.h"


typedef struct _GayAttitudeDelayedMessageRequest GayAttitudeDelayedMessageRequest;
typedef struct _GayAttitudeConversationInfo GayAttitudeConversationInfo;

struct _GayAttitudeDelayedMessageRequest {
	GayAttitudeAccount	*gaa;
	GayAttitudeBuddy	*gabuddy;
	gchar			*what;
	PurpleMessageFlags	flags;
};

struct _GayAttitudeConversationInfo {
	guint64			latest_msg_id;
	gchar			*url_path, *checksum;
};

int ga_message_send(GayAttitudeAccount *gaa, GayAttitudeBuddy *gabuddy, const char *what, PurpleMessageFlags flags);
void ga_message_check_received(GayAttitudeAccount *gaa);

#endif /* GA_GAMESSAGE_H */
