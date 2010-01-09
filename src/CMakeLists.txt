SET(BIN_NAME minbif)

IF(PAM_FOUND)
	SET(MINBIF_EXTRA_FILES_PAM "im/user_pam.cpp")
ENDIF(PAM_FOUND)
ADD_EXECUTABLE(${BIN_NAME}
		core/minbif.cpp
		core/sighandler.cpp
		core/util.cpp
		core/log.cpp
		core/mutex.cpp
		core/callback.cpp
		core/config.cpp
		core/caca_image.cpp
		server_poll/poll.cpp
		server_poll/inetd.cpp
		server_poll/daemon_fork.cpp
		im/im.cpp
		im/user.cpp
		im/user_local.cpp
		${MINBIF_EXTRA_FILES_PAM}
		im/protocol.cpp
		im/account.cpp
		im/roomlist.cpp
		im/buddy.cpp
		im/conversation.cpp
		im/request.cpp
		im/ft.cpp
		im/media.cpp
		im/purple.cpp
		irc/settings.cpp
		irc/irc.cpp
		irc/dcc.cpp
		irc/message.cpp
		irc/server.cpp
		irc/nick.cpp
		irc/user.cpp
		irc/buddy_icon.cpp
		irc/buddy.cpp
		irc/unknown_buddy.cpp
		irc/chat_buddy.cpp
		irc/channel.cpp
		irc/status_channel.cpp
		irc/conversation_channel.cpp
	      )

TARGET_LINK_LIBRARIES(${BIN_NAME} "-lpthread -lstdc++" ${PURPLE_LDFLAGS} ${CACA_LDFLAGS} ${IMLIB_LDFLAGS} ${GSTREAMER_LDFLAGS} ${FARSIGHT_LDFLAGS} ${PAM_LDFLAGS})

INSTALL(TARGETS ${BIN_NAME}
        DESTINATION bin)
