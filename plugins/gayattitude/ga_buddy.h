#ifndef GA_GABUDDY_H
#define GA_GABUDDY_H

#include <purple.h>
#include "ga_account.h"


typedef struct _GayAttitudeBuddy GayAttitudeBuddy;

struct _GayAttitudeBuddy {
	PurpleBuddy		*buddy;
	gchar			*ref_id;
};


GayAttitudeBuddy *ga_get_gabuddy_from_buddy(PurpleBuddy *buddy, gboolean create);
GayAttitudeBuddy *ga_find_gabuddy(GayAttitudeAccount *gaa, const gchar *gabuddyname);
GayAttitudeBuddy *ga_gabuddy_new(GayAttitudeAccount *gaa, const gchar *buddyname);
void ga_gabuddy_free(GayAttitudeBuddy *gabuddy);

#endif /* GA_GABUDDY_H */
