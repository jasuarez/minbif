ADD_LIBRARY(gayattitude SHARED
		ga_account.c
		ga_buddy.c
		ga_buddylist.c
		gayattitude.c
		)

TARGET_LINK_LIBRARIES(gayattitude ${PURPLE_LDFLAGS} ${LIBXML_LDFLAGS} pluginlib)
