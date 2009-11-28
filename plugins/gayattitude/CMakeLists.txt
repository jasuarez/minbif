ADD_LIBRARY(gayattitude SHARED
		ga_buddy.c
		gayattitude.c
		)

TARGET_LINK_LIBRARIES(gayattitude ${PURPLE_LDFLAGS} ${LIBXML_LDFLAGS} pluginlib)
