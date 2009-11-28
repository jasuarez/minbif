#ifndef GA_GAMESSAGE_H
#define GA_GAMESSAGE_H

#include <purple.h>
#include "ga_account.h"
#include "ga_buddy.h"


typedef struct _GayAttitudeDelayedMessageRequest GayAttitudeDelayedMessageRequest;

struct _GayAttitudeDelayedMessageRequest {
	GayAttitudeAccount	*gaa;
	GayAttitudeBuddy	*gabuddy;
	gchar			*what;
	PurpleMessageFlags	flags;
};

int ga_message_send(GayAttitudeAccount *gaa, GayAttitudeBuddy *gabuddy, const char *what, PurpleMessageFlags flags);

#endif /* GA_GAMESSAGE_H */
