SET(BIN_NAME minbif)

IF(PAM_FOUND)
	SET(MINBIF_EXTRA_FILES_PAM "im/auth_pam.cpp")
ENDIF(PAM_FOUND)
IF(GNUTLS_FOUND)
	SET(MINBIF_EXTRA_FILES_TLS "sockwrap/sockwrap_tls.cpp")
ENDIF(GNUTLS_FOUND)
ADD_EXECUTABLE(${BIN_NAME}
		core/minbif.cpp
		core/sighandler.cpp
		core/util.cpp
		core/log.cpp
		core/mutex.cpp
		core/callback.cpp
		core/config.cpp
		core/caca_image.cpp
		sockwrap/sockwrap.cpp
		sockwrap/sockwrap_plain.cpp
		${MINBIF_EXTRA_FILES_TLS}
		server_poll/poll.cpp
		server_poll/inetd.cpp
		server_poll/daemon_fork.cpp
		im/im.cpp
		im/auth.cpp
		im/auth_local.cpp
		im/auth_connection.cpp
		${MINBIF_EXTRA_FILES_PAM}
		im/plugin.cpp
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
		irc/cmds.cpp
		irc/cmds_accounts.cpp
		irc/cmds_channels.cpp
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

TARGET_LINK_LIBRARIES(${BIN_NAME} "-lpthread -lstdc++" ${PURPLE_LIBRARIES} ${GTHREAD_LIBRARIES} ${CACA_LIBRARIES} ${IMLIB_LIBRARIES} ${GSTREAMER_LIBRARIES} ${FARSIGHT_LIBRARIES} ${PAM_LIBRARIES} ${GNUTLS_LIBRARIES})

INSTALL(TARGETS ${BIN_NAME}
        DESTINATION bin)
