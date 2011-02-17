# This script renames your Facebook buddies to a readable format when 
# using Facebook's XMPP gateway with Minbif. 

# Based on the Irssi script at http://browsingtheinternet.com/temp/bitlbee_rename.txt 
# Ported for Weechat 0.3.0 or later by Jaakko Lintula (crwl@iki.fi)
# Modified for Minbif by 'varogami' <varogami@gmail.com>
# 
# This program can be distributed under the terms of the GNU GPL3.
# Testing contrib 'bizio' maestrozappa@gmail.com

#Edit this variables with your own minbif configuration settings
minbifChannel = "&minbif"
minbifServer = "minbif"
facebookhostname = "chat.facebook.com"

minbifBuffer = "%s.%s" % (minbifServer, minbifChannel)
nicksToRename = set()

import weechat
import re

weechat.register("facebook_rename_minbif", "varogami", "0.0.2", "GPL", "Renames Facebook usernames when using minbif", "", "")

def message_join(data, signal, signal_data):
  signal_data = signal_data.split()
  channel = signal_data[2]
  hostmask = signal_data[0]
  nick = hostmask[1:hostmask.index('!')]
  username = hostmask[hostmask.index('!')+1:hostmask.index('@')]
  server = hostmask[hostmask.index('@')+1:]
  server = server[:+server.index(':')]
  
  if channel == minbifChannel and nick == username and nick[0] == '-' and server == facebookhostname:
   nicksToRename.add(nick)
   weechat.command(weechat.buffer_search("irc", minbifBuffer), "/whois "+nick+" "+nick)
   
  return weechat.WEECHAT_RC_OK

def whois_data(data, signal, signal_data):
  if "Full Name" in signal_data:
   nick = signal_data.split("Full Name:")[0].strip()
   nick = nick[1:nick.index(' :')]
   nick = nick.split(' ')
   nick = nick[3]
   realname =  signal_data.split("Full Name:")[1].strip()
  
   if nick in nicksToRename:
     nicksToRename.remove(nick)   
     ircname = re.sub("[^A-Za-z0-9]", "", realname)[:24]
     if ircname != nick:
       weechat.command(weechat.buffer_search("irc", minbifBuffer), "/quote -server %s svsnick %s %s" % (minbifServer, nick, ircname))

  return weechat.WEECHAT_RC_OK

weechat.hook_signal(minbifServer+",irc_in_join", "message_join", "")
weechat.hook_signal(minbifServer+",irc_in_320", "whois_data", "")
