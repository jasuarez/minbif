ADD_LIBRARY(gayattitude SHARED
		gayattitude.c
		connection.c)

TARGET_LINK_LIBRARIES(gayattitude ${PURPLE_LDFLAGS})
