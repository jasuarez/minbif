ADD_LIBRARY(coincoin SHARED
		coincoin.c
		connection.c)

TARGET_LINK_LIBRARIES(coincoin ${PURPLE_LDFLAGS})
