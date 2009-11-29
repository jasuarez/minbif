ADD_LIBRARY(gayattitude SHARED
		ga_parsing.c
		ga_account.c
		ga_buddy.c
		ga_buddylist.c
		ga_message.c
		gayattitude.c
		)

TARGET_LINK_LIBRARIES(gayattitude ${PURPLE_LDFLAGS} ${LIBXML_LDFLAGS} pluginlib)
