#ifndef GA_GAACCOUNT_H
#define GA_GAACCOUNT_H

#include <purple.h>
#include "../lib/http.h"

typedef struct _GayAttitudeAccount GayAttitudeAccount;

struct _GayAttitudeAccount {
	PurpleAccount *account;
	PurpleConnection *pc;
	HttpHandler* http_handler;
	guint new_messages_check_timer;
	GHashTable *ref_ids;
};

#endif /* GA_GAACCOUNT_H */
