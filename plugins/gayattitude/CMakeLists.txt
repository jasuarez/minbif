ADD_LIBRARY(gayattitude SHARED
		gayattitude.c
		)

TARGET_LINK_LIBRARIES(gayattitude ${PURPLE_LDFLAGS} ${LIBXML_LDFLAGS} pluginlib)
