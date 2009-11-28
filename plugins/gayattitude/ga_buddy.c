
#include "ga_buddy.h"


GayAttitudeBuddy *ga_get_gabuddy_from_buddy(PurpleBuddy *buddy, gboolean create)
{
	GayAttitudeBuddy *gabuddy;

	if (!buddy)
		return NULL;

	gabuddy = buddy->proto_data;
	if (!gabuddy && create)
	{
		gabuddy = g_new0(GayAttitudeBuddy, TRUE);
		gabuddy->buddy = buddy;
		gabuddy->ref_id = NULL;
	}

	return gabuddy;
}

GayAttitudeBuddy *ga_find_gabuddy(GayAttitudeAccount *gaa, const gchar *buddyname)
{
	PurpleBuddy *buddy;
	GayAttitudeBuddy *gabuddy;

	buddy = purple_find_buddy(gaa->account, buddyname);
	if (!buddy)
		return NULL;

	gabuddy = ga_get_gabuddy_from_buddy(buddy, TRUE);

	return gabuddy;
}

GayAttitudeBuddy *ga_gabuddy_new(GayAttitudeAccount *gaa, const gchar *buddyname)
{
	PurpleBuddy *buddy;

	buddy = purple_buddy_new(gaa->account, buddyname, NULL);
	return ga_get_gabuddy_from_buddy(buddy, TRUE);
}

void ga_gabuddy_free(GayAttitudeBuddy *gabuddy)
{
	if (!gabuddy)
		return;

	g_free(gabuddy);
}

