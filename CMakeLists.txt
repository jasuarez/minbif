PROJECT(bitlbee2)
cmake_minimum_required(VERSION 2.6)

SET(BIN_NAME bitlbee2)
ADD_EXECUTABLE(${BIN_NAME}
		src/bitlbee.cpp
		src/util.cpp
		src/log.cpp
		src/mutex.cpp
		src/callback.cpp
		src/config.cpp
		src/server_poll/poll.cpp
		src/server_poll/inetd.cpp
		src/irc/irc.cpp
		src/irc/message.cpp
		src/irc/server.cpp
		src/irc/nick.cpp
		src/irc/user.cpp
		src/irc/channel.cpp
	      )
SET(CONF_NAME bitlbee.conf)

target_link_libraries(${BIN_NAME})

## Purple
SET(PURPLE_FIND_REQUIRED TRUE)
INCLUDE(UsePkgConfig)
PKGCONFIG("purple" PURPLE_INCLUDE_DIR PURPLE_LINK_DIR
	        PURPLE_LDFLAGS PURPLE_CFLAGS)
IF(PURPLE_INCLUDE_DIR OR PURPLE_LINK_DIR OR PURPLE_LDFLAGS OR PURPLE_CFLAGS)
  SET(PURPLE_FOUND TRUE)
ELSE(PURPLE_INCLUDE_DIR OR PURPLE_LINK_DIR OR PURPLE_LDFLAGS OR PURPLE_CFLAGS)
  SET(PURPLE_FOUND FALSE)
ENDIF(PURPLE_INCLUDE_DIR OR PURPLE_LINK_DIR OR PURPLE_LDFLAGS OR PURPLE_CFLAGS)
IF(PURPLE_FOUND)
	MESSAGE(STATUS "Found libpurple: ${PURPLE_LINK_DIR} ${PURPLE_LDFLAGS}")
ELSE(PURPLE_FOUND)
	MESSAGE(FATAL_ERROR "Unable to find purple library")
ENDIF(PURPLE_FOUND)


INCLUDE_DIRECTORIES(${PURPLE_INCLUDE_DIR} "src/")
LINK_DIRECTORIES(${PURPLE_LINK_DIR})
TARGET_LINK_LIBRARIES(${BIN_NAME} "-lpthread -lstdc++" ${PURPLE_LDFLAGS})


SET(CMAKE_BUILD_TYPE ${BUILD})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PURPLE_CFLAGS} -DDEBUG -D_REENTRANT -D_FILE_OFFSET_BITS=64 -Werror -Wall -Wextra -Wno-unused-parameter -Wconversion")
SET(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})

#STRING(COMPARE EQUAL ${CMAKE_BUILD_TYPE} "Debug" DEBUG_BUILD)
#IF(DEBUG_BUILD)
#	MESSAGE(STATUS "We'll perform tests during compilation")
#	# Looks a bit like a hack.. should find a better way to do this
#	SET(CMAKE_C_FLAGS "${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS}")
#	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS}")
#	SET(CMAKE_C_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/tools/gcc-wrapper/my-gcc.sh")
#	SET(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/tools/gcc-wrapper/my-gcc.sh")
#ENDIF(DEBUG_BUILD)

MESSAGE(STATUS "Using compiler ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

INSTALL(TARGETS ${BIN_NAME}
        DESTINATION bin)
INSTALL(FILES ${CONF_NAME}
        DESTINATION etc/bitlbee)
INSTALL(FILES BUGS COPYING HACKING README ROADMAP TASKS
        DESTINATION doc/bitlbee)
INSTALL(DIRECTORY doc/
        DESTINATION doc/bitlbee)
