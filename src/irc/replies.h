/*
 * Minbif - IRC instant messaging gateway
 * Copyright(C) 2009 Romain Bignon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef IRC_REPLIES_H
#define IRC_REPLIES_H

#define RPL_WELCOME          "001"
#define RPL_YOURHOST         "002"
#define RPL_CREATED          "003"
#define RPL_MYINFO           "004"
#define RPL_ISUPPORT         "005"
#define RPL_MAP              "015"
#define RPL_MAPEND           "017"
#define RPL_STATSCOMMANDS    "212"
#define RPL_ENDOFSTATS       "219"
#define RPL_UMODEIS          "221"
#define RPL_STATSUPTIME      "242"
#define RPL_LUSERCLIENT      "251"
#define RPL_LUSEROP          "252"
#define RPL_LUSERUNKNOWN     "253"
#define RPL_LUSERCHANNELS    "254"
#define RPL_LUSERME          "255"
#define RPL_ADMINME          "256"
#define RPL_AWAY             "301"
#define RPL_ISON             "303"
#define RPL_UNAWAY           "305"
#define RPL_NOWAWAY          "306"
#define RPL_WHOISUSER        "311"
#define RPL_WHOISSERVER      "312"
#define RPL_WHOISOPERATOR    "313"
#define RPL_ENDOFWHO         "315"
#define RPL_ENDOFWHOIS       "318"
#define RPL_WHOISCHANNELS    "319"
#define RPL_WHOISACTUALLY    "320"
#define RPL_LISTSTART        "321"
#define RPL_LIST             "322"
#define RPL_LISTEND          "323"
#define RPL_CHANNELMODEIS    "324"
#define RPL_CREATIONTIME     "329"
#define RPL_NOTOPIC          "331"
#define RPL_TOPIC            "332"
#define RPL_INVITING         "341"
#define RPL_VERSION          "351"
#define RPL_WHOREPLY         "352"
#define RPL_NAMREPLY         "353"
#define RPL_ENDOFNAMES       "366"
#define RPL_BANLIST          "367"
#define RPL_ENDOFBANLIST     "368"
#define RPL_ENDOFWHOWAS      "369"
#define RPL_INFO             "371"
#define RPL_MOTD             "372"
#define RPL_ENDOFINFO        "374"
#define RPL_MOTDSTART        "375"
#define RPL_ENDOFMOTD        "376"
#define RPL_YOUREOPER        "381"
#define RPL_REHASHING        "382"

#define ERR_NOSUCHNICK       "401"
#define ERR_NOSUCHCHANNEL    "403"
#define ERR_WASNOSUCHNICK    "406"
#define ERR_UNKNOWNCOMMAND   "421"
#define ERR_NONICKNAMEGIVEN  "431"
#define ERR_ERRONEUSNICKNAME "432"
#define ERR_NICKNAMEINUSE    "433"
#define ERR_NICKTOOFAST      "438"
#define ERR_NOTONCHANNEL     "442"
#define ERR_NOTREGISTERED    "451"
#define ERR_NEEDMOREPARAMS   "461"
#define ERR_ALREADYREGISTRED "462"
#define ERR_PASSWDMISMATCH   "464"
#define ERR_CHANFORWARDING   "470"
#define ERR_NOPRIVILEGES     "481"
#define ERR_CHANOPRIVSNEEDED "482"
#define ERR_UMODEUNKNOWNFLAG "501"

#define MSG_PRIVMSG          "PRIVMSG"
#define MSG_NOTICE           "NOTICE"
#define MSG_MODE             "MODE"
#define MSG_JOIN             "JOIN"
#define MSG_PART             "PART"
#define MSG_QUIT             "QUIT"
#define MSG_ERROR            "ERROR"
#define MSG_NICK             "NICK"
#define MSG_PING             "PING"
#define MSG_PONG             "PONG"
#define MSG_USER             "USER"
#define MSG_PASS             "PASS"
#define MSG_VERSION          "VERSION"
#define MSG_INFO             "INFO"
#define MSG_WHO              "WHO"
#define MSG_WHOIS            "WHOIS"
#define MSG_WHOWAS           "WHOWAS"
#define MSG_STATS            "STATS"
#define MSG_CONNECT          "CONNECT"
#define MSG_SQUIT            "SQUIT"
#define MSG_MAP		     "MAP"
#define MSG_ADMIN            "ADMIN"
#define MSG_LIST             "LIST"
#define MSG_ISON             "ISON"
#define MSG_INVITE           "INVITE"
#define MSG_KICK             "KICK"
#define MSG_KILL             "KILL"
#define MSG_SVSNICK          "SVSNICK"
#define MSG_TOPIC            "TOPIC"
#define MSG_NAMES            "NAMES"
#define MSG_AWAY             "AWAY"
#define MSG_MOTD             "MOTD"
#define MSG_WALLOPS          "WALLOPS"
#define MSG_REHASH           "REHASH"
#define MSG_DIE              "DIE"
#define MSG_OPER             "OPER"
#define MSG_CMD              "CMD"

#endif /* IRC_REPLIES_H */
