PROJECT(minbif)
cmake_minimum_required(VERSION 2.6)

SET(BIN_NAME minbif)
ADD_EXECUTABLE(${BIN_NAME}
		src/minbif.cpp
		src/util.cpp
		src/log.cpp
		src/mutex.cpp
		src/callback.cpp
		src/config.cpp
		src/caca_image.cpp
		src/server_poll/poll.cpp
		src/server_poll/inetd.cpp
		src/server_poll/daemon_fork.cpp
		src/im/im.cpp
		src/im/protocol.cpp
		src/im/account.cpp
		src/im/buddy.cpp
		src/im/conversation.cpp
		src/im/request.cpp
		src/im/ft.cpp
		src/im/media.cpp
		src/im/purple.cpp
		src/irc/settings.cpp
		src/irc/irc.cpp
		src/irc/dcc.cpp
		src/irc/message.cpp
		src/irc/server.cpp
		src/irc/nick.cpp
		src/irc/user.cpp
		src/irc/buddy_icon.cpp
		src/irc/buddy.cpp
		src/irc/chat_buddy.cpp
		src/irc/channel.cpp
		src/irc/status_channel.cpp
		src/irc/conversation_channel.cpp
	      )
SET(CONF_NAME minbif.conf)

target_link_libraries(${BIN_NAME})

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(PURPLE REQUIRED purple>=2.4)

OPTION(ENABLE_CACA "Enable libcaca library" ON)
IF(ENABLE_CACA)
	PKG_CHECK_MODULES(CACA caca>=0.99)
	PKG_CHECK_MODULES(IMLIB imlib2>=1.0)

	IF(IMLIB_FOUND AND CACA_FOUND)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_CACA")
	ELSE(IMLIB_FOUND AND CACA_FOUND)
		MESSAGE(FATAL_ERROR "Unable to find the libcaca library. To disable caca output, run 'make ENABLE_CACA=0'")
	ENDIF(IMLIB_FOUND AND CACA_FOUND)

ENDIF(ENABLE_CACA)

OPTION(ENABLE_VIDEO "Enable video support" ON)
IF(ENABLE_VIDEO)
	PKG_CHECK_MODULES(GSTREAMER gstreamer-0.10>=0.10.10 gstreamer-interfaces-0.10>=0.10)
	PKG_CHECK_MODULES(FARSIGHT farsight2-0.10>=0.0.9)
	PKG_CHECK_MODULES(PURPLE_26 purple>=2.6.1)
	IF(GSTREAMER_FOUND AND PURPLE_26_FOUND AND FARSIGHT_FOUND)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_VIDEO")
	ELSE(GSTREAMER_FOUND AND PURPLE_26_FOUND AND FARSIGHT_FOUND)
		MESSAGE(FATAL_ERROR "Dependencies for video were not met. Install gstreamer, farsight and libpurple>=2.6 first. Or to disable video, run 'make ENABLE_VIDEO=0'")
	ENDIF(GSTREAMER_FOUND AND PURPLE_26_FOUND AND FARSIGHT_FOUND)
ENDIF(ENABLE_VIDEO)

INCLUDE_DIRECTORIES(${PURPLE_INCLUDE_DIRS} ${CACA_INCLUDE_DIRS} ${IMLIB_INCLUDE_DIRS} ${GSTREAMER_INCLUDE_DIRS} ${FARSIGHT_INCLUDE_DIRS} "src/")
LINK_DIRECTORIES(${PURPLE_LINK_DIRS} ${CACA_LINK_DIRS} ${IMLIB_LINK_DIRS} ${GSTREAMER_LINK_DIRS} ${FARSIGHT_LINK_DIRS})
TARGET_LINK_LIBRARIES(${BIN_NAME} "-lpthread -lstdc++" ${PURPLE_LDFLAGS} ${CACA_LDFLAGS} ${IMLIB_LDFLAGS} ${GSTREAMER_LDFLAGS} ${FARSIGHT_LDFLAGS})


SET(CMAKE_BUILD_TYPE ${BUILD})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG -D_REENTRANT -D_FILE_OFFSET_BITS=64 -Werror -Wall -Wextra -Wno-unused-parameter")
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

IF(NOT CONF_PREFIX)
	SET(CONF_PREFIX ${CMAKE_INSTALL_PREFIX}/etc/minbif)
ENDIF(NOT CONF_PREFIX)
IF(NOT MAN_INSTALL_PATH)
	SET(MAN_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/share/man/)
ENDIF(NOT MAN_INSTALL_PATH)

MESSAGE(STATUS "Using compiler ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

INSTALL(FILES man/minbif.8 DESTINATION ${MAN_INSTALL_PATH}/man8)
INSTALL(TARGETS ${BIN_NAME}
        DESTINATION bin)
INSTALL(FILES COPYING README
        DESTINATION doc/minbif)
INSTALL(DIRECTORY doc/
        DESTINATION doc/minbif)
INSTALL(FILES ${CONF_NAME}
        DESTINATION ${CONF_PREFIX})
