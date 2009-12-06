#ifndef GA_GAACCOUNT_H
#define GA_GAACCOUNT_H

#include <purple.h>
#include "../lib/http.h"


#define GA_CHECK_INTERVAL 30

typedef struct _GayAttitudeAccount GayAttitudeAccount;

struct _GayAttitudeAccount {
	PurpleAccount *account;
	PurpleConnection *pc;
	HttpHandler* http_handler;
	guint new_messages_check_timer;
	guint64 latest_msg_id;
	GHashTable *conv_info;
	GHashTable *conv_with_buddy_count;
};

GayAttitudeAccount* ga_account_new(PurpleAccount *account);
void ga_account_free(GayAttitudeAccount* gaa);
void ga_account_login(GayAttitudeAccount *gaa);
void ga_account_logout(GayAttitudeAccount *gaa);

#endif /* GA_GAACCOUNT_H */
