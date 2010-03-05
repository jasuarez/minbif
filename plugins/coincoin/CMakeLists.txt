ADD_LIBRARY(coincoin SHARED
		coincoin.c
		message.c
		)

TARGET_LINK_LIBRARIES(coincoin ${PURPLE_LIBRARIES} pluginlib)
INSTALL(TARGETS coincoin
	DESTINATION lib/purple-2)
