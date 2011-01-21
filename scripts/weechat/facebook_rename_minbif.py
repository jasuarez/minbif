# This script renames your Facebook buddies to a readable format when
# using Facebook's XMPP gateway with Minbif.
#
# Based on the bitlbee rename script
# Copyright (C) 2010  varogami  <varogami@gmail.com>
#
# This program can be distributed under the terms of the GNU GPL.
# Testing contrib 'bizio' maestrozappa@gmail.com

minbifChannel = "minbif"
minbifServer = "minbif"

minbifBuffer = "%s.%s" % (minbifServer, minbifChannel)
facebookhostname = "chat.facebook.com"
nicksToRename = set()

import weechat
import re

weechat.register("facebook_rename_minbif", "varogami", "0.0.1", "GPL", "Renames Facebook usernames when using minbif", "", "")

def message_join(data, signal, signal_data):
  signal_data = signal_data.split()
  channel = signal_data[2][1:]
  hostmask = signal_data[0]
  nick = hostmask[1:hostmask.index('!')]
  username = hostmask[hostmask.index('!')+1:hostmask.index('@')]
  server = hostmask[hostmask.index('@')+1:]
  server = server[:+server.index(':')]

  #if channel == minbifChannel and nick == username and nick[0] == '-' and server == facebookhostname:
  if channel == minbifChannel and nick == username and nick[0] == '-' and server == facebookhostname:
   nicksToRename.add(nick)
   weechat.command(weechat.buffer_search("irc", minbifBuffer), "/whois " +nick)
   #weechat.prnt(weechat.current_buffer(), "server:"+server)
   #for testing

  return weechat.WEECHAT_RC_OK

def whois_data(data, signal, signal_data):
  nick = signal_data.split()[3]
  realname = signal_data[signal_data.index(' :')+2:]
  realname = realname[:+realname.index('[')]

  if nick in nicksToRename:
    nicksToRename.remove(nick)
    ircname = re.sub("[^A-Za-z0-9]", "", realname)[:24]
    if ircname != nick:
      weechat.command(weechat.buffer_search("irc", minbifBuffer), "/quote -server %s svsnick %s %s" % (minbifServer, nick, ircname))
      #weechat.prnt(weechat.current_buffer(), "nick:"+nick+ " realname:"+realname)
      #for testing

  return weechat.WEECHAT_RC_OK

weechat.hook_signal("*,irc_in_join", "message_join", "")
weechat.hook_signal("*,irc_in_311", "whois_data", "")
