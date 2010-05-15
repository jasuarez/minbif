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

#include <cassert>
#include <cstring>
#ifdef HAVE_IMLIB
	#include <Imlib2.h>
#endif /* HAVE_IMLIB */

#include "im/im.h"
#include "im/account.h"
#include "im/conversation.h"
#include "im/buddy.h"
#include "im/purple.h"
#include "core/log.h"
#include "core/version.h"
#include "irc/irc.h"
#include "irc/status_channel.h"
#include "irc/user.h"
#include "irc/buddy.h"

namespace im {

Account::Account()
	: account(NULL)
{}

Account::Account(PurpleAccount* _account, Protocol _proto)
	: account(_account),
	  proto(_proto)
{
	if(account && !proto.isValid())
	{
		if(account && account->gc)
			proto = Protocol(account->gc->prpl);
		else
			proto = Purple::getProtocolByPurpleID(account->protocol_id);
	}
}

bool Account::operator==(const Account& account) const
{
	return this->isValid() && account.isValid() && account.account == this->account;
}

bool Account::operator!=(const Account& account) const
{
	return !isValid() || !account.isValid() || account.account != this->account;
}

string Account::getUsername() const
{
	assert(isValid());
	return account->username;
}

string Account::getPassword() const
{
	assert(isValid());
	return p2s(purple_account_get_password(account));
}

void Account::setPassword(string password)
{
	assert(isValid());
	purple_account_set_password(account, password.c_str());
}

void Account::registerAccount() const
{
	assert(isValid());
	purple_account_register(account);
}

vector<string> Account::getCommandsList() const
{
	assert(isValid());
	GList *actions, *l;
	vector<string> cmds;
	PurpleConnection *gc = purple_account_get_connection(account);
	PurplePlugin *plugin = (gc && PURPLE_CONNECTION_IS_CONNECTED(gc)) ? gc->prpl : NULL;

	if(!plugin || !PURPLE_PLUGIN_HAS_ACTIONS(plugin))
		return cmds;

	actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

	for (l = actions; l != NULL; l = l->next)
		if (l->data)
			cmds.push_back(irc::Nick::nickize(((PurplePluginAction *) l->data)->label));

	return cmds;
}

bool Account::callCommand(const string& cmd) const
{
	assert(isValid());
	GList *actions, *l;
	PurpleConnection *gc = purple_account_get_connection(account);
	PurplePlugin *plugin = (gc && PURPLE_CONNECTION_IS_CONNECTED(gc)) ? gc->prpl : NULL;

	if(!plugin || !PURPLE_PLUGIN_HAS_ACTIONS(plugin))
		return false;

	actions = PURPLE_PLUGIN_ACTIONS(plugin, gc);

	for (l = actions; l != NULL; l = l->next)
	{
		PurplePluginAction* action = (PurplePluginAction*)l->data;
		if (l->data && irc::Nick::nickize(action->label) == cmd)
		{
			action->plugin = plugin;
			action->context = gc;
			action->callback(action);
			return true;
		}
	}

	return false;
}

Protocol::Options Account::getOptions() const
{
	assert(isValid());

	Protocol::Options options = getProtocol().getOptions();
	FOREACH(Protocol::Options, options, it)
	{
		Protocol::Option& option = it->second;
		switch(option.getType())
		{
			case Protocol::Option::STR:
				option.setValue(purple_account_get_string(account, option.getName().c_str(), option.getValue().c_str()));
				break;
			case Protocol::Option::INT:
				option.setValue(t2s(purple_account_get_int(account, option.getName().c_str(), option.getValueInt())));
				break;
			case Protocol::Option::BOOL:
				option.setValue(purple_account_get_bool(account, option.getName().c_str(), option.getValueBool()) ? "true" : "false");
				break;
			case Protocol::Option::ACCID:
				option.setValue(getID());
				break;
			case Protocol::Option::STATUS_CHANNEL:
				option.setValue(getStatusChannelName());
				break;
			case Protocol::Option::PASSWORD:
				option.setValue(getPassword());
				break;
			case Protocol::Option::SERVER_ALIASES:
				option.setValue(hasServerAliases() ? "true" : "false");
				break;
			case Protocol::Option::NONE:
				/* not supported. */
				break;
		}
	}
	return options;
}

void Account::setOptions(const Protocol::Options& options)
{
	assert(isValid());
	for(Protocol::Options::const_iterator it = options.begin(); it != options.end(); ++it)
	{
		const Protocol::Option& option = it->second;

		switch(option.getType())
		{
			case Protocol::Option::STR:
				purple_account_set_string(account,
							  option.getName().c_str(),
							  option.getValue().c_str());
				break;
			case Protocol::Option::INT:
				purple_account_set_int(account,
						       option.getName().c_str(),
						       option.getValueInt());
				break;
			case Protocol::Option::BOOL:
				purple_account_set_bool(account,
						        option.getName().c_str(),
							option.getValueBool());
				break;
			case Protocol::Option::ACCID:
			{
				string value = option.getValue();
				if(getID() == value)
					break;

				if(isConnected())
					throw Protocol::OptionError("Can't change ID of a connected account");

				if(value.empty())
					value = Purple::getNewAccountName(proto, *this);
				else if(Purple::getIM()->getAccount(value).isValid() == true)
					throw Protocol::OptionError("This ID is already used.");

				setID(value);
				break;
			}
			case Protocol::Option::STATUS_CHANNEL:
				if(getStatusChannelName() == option.getValue())
					break;

				if(!irc::Channel::isChanName(option.getValue()) || !irc::Channel::isStatusChannel(option.getValue()))
					throw Protocol::OptionError("'" + option.getValue() + "' is not a valid channel name");
				setStatusChannelName(option.getValue());
				break;
			case Protocol::Option::PASSWORD:
				if (option.getValue() == getPassword())
					break;

				if (option.getValue().empty())
					purple_account_set_remember_password(account, FALSE);
				else
				{
					setPassword(option.getValue());
					purple_account_set_remember_password(account, TRUE);
				}
				break;
			case Protocol::Option::SERVER_ALIASES:
				setServerAliases(option.getValueBool());
				break;
			case Protocol::Option::NONE:
				/* Not happy. */
				break;
		}
	}
}

void Account::setBuddyIcon(string filename)
{
	assert(isValid());
	PurplePlugin *plug = purple_find_prpl(purple_account_get_protocol_id(account));
	if (plug) {
		PurplePluginProtocolInfo *prplinfo = PURPLE_PLUGIN_PROTOCOL_INFO(plug);
		if (prplinfo != NULL &&
		    //purple_account_get_bool(account, "use-global-buddyicon", TRUE) &&
		    prplinfo->icon_spec.format)
		{
			GError* temp_error = NULL;
#ifdef HAVE_IMLIB
			Imlib_Image img = imlib_load_image(filename.c_str());
			char* temp_filename = NULL;

			if (img) {
				int temp_fd;

				/* Create a stupid tmp file, write it, close it. Save image as png in it. Fuck it. */
				temp_fd = g_file_open_tmp("minbif_new_icon_XXXXXX", &temp_filename, &temp_error);
				if (temp_error) {
					b_log[W_ERR] << "Unable to create a temporary file: " << temp_error->message;
					g_error_free(temp_error);
					temp_filename = NULL;
				}
				else
				{
					char** prpl_formats = g_strsplit(prplinfo->icon_spec.format,",",0);
					ImlibLoadError err = IMLIB_LOAD_ERROR_UNKNOWN;

					close(temp_fd);
					/* Try to encode in a supported format. */
					imlib_context_set_image(img);
					for (size_t i = 0; prpl_formats[i] && err != IMLIB_LOAD_ERROR_NONE; ++i)
					{
						imlib_image_set_format(prpl_formats[i]);
						imlib_save_image_with_error_return(temp_filename, &err);
					}

					if (err != IMLIB_LOAD_ERROR_NONE)
						b_log[W_ERR] << "Unable to encode image for " << getID();
					else
						filename = temp_filename;
					g_strfreev(prpl_formats);
				}
				imlib_free_image();
			}

#endif /* HAVE_IMLIB */
			gchar *contents;
			gsize len;
			if (g_file_get_contents(filename.c_str(), &contents, &len, &temp_error))
			{
				if ((prplinfo->icon_spec.max_filesize != 0) &&
				    (len > prplinfo->icon_spec.max_filesize))
				{
					b_log[W_ERR] << "Unable to set image on " << getID() << ": image too large";
				}
				else
				{
					purple_buddy_icons_set_account_icon(account, (guchar *)contents, len);
					/* Useless, and a temp filename is not safe:
					 *   purple_account_set_buddy_icon_path(account, temp_filename);
					 */
				}
			}
			else if (temp_error)
			{
				b_log[W_ERR] << "Unable to get image content on " << getID() << ": " << temp_error->message;
				g_error_free(temp_error);
			}
#ifdef HAVE_IMLIB
			if (temp_filename) {
				unlink(temp_filename);
				g_free(temp_filename);
			}
#endif /* HAVE_IMLIB */
		}
	}

}

void Account::setStatus(PurpleStatusPrimitive prim, string message)
{
	assert(isValid());

	const char *id;
	PurpleStatus* status;

	if (prim == PURPLE_STATUS_UNSET)
	{
		status = purple_account_get_active_status(account);
		id = purple_status_get_id(status);
	}
	else
	{
		id = purple_primitive_get_id_from_type(prim);
		status = purple_account_get_status(account, id);
	}

	if (!status)
		return;

	irc::StatusChannel* chan = getStatusChannel();
	if (message.empty() && chan)
		message = chan->getTopic();
	if (message.empty() && prim != PURPLE_STATUS_UNSET)
	{
		/* Get the status' message only if we change it. */
		message = p2s(purple_status_get_attr_string(status, "message"));
		if (!message.empty() && chan && purple_primitive_get_type_from_id(id) == PURPLE_STATUS_AVAILABLE)
			chan->setTopic(NULL, message);
	}

	purple_account_set_status(account, id, TRUE, "message", message.c_str(), (char*)NULL);
}

PurpleStatusPrimitive Account::getStatus() const
{
	assert(isValid());

	PurpleStatus *s = purple_account_get_active_status(account);
	if (!s)
		return PURPLE_STATUS_UNSET;

	return purple_primitive_get_type_from_id(purple_status_get_id(s));
}

string Account::getStatusMessage(PurpleStatusPrimitive prim) const
{
	assert(isValid());

	PurpleStatus *s;
	if (prim == PURPLE_STATUS_UNSET)
		s = purple_account_get_active_status(account);
	else
		s = purple_account_get_status(account, purple_primitive_get_id_from_type(prim));

	if (!s)
		return "";

	return p2s(purple_status_get_attr_string(s, "message"));
}

void Account::setID(string id) const
{
	assert(isValid());
	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "id", id.c_str());
}

string Account::getID() const
{
	assert(isValid());
	string n = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "id", "");
	if(n.empty())
	{
		n = Purple::getNewAccountName(proto, *this);
		setID(n);
	}
	return n;
}

bool Account::hasServerAliases() const
{
	assert(isValid());
	return purple_account_get_ui_bool(account, MINBIF_VERSION_NAME, "server_aliases", true);
}

void Account::setServerAliases(bool b)
{
	assert(isValid());
	purple_account_set_ui_bool(account, MINBIF_VERSION_NAME, "server_aliases", b);
}

string Account::getStatusChannelName() const
{
	assert(isValid());
	string n = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "channel", "");
	return n;
}

void Account::setStatusChannelName(const string& c)
{
	assert(isValid());

	leaveStatusChannel();
	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "channel", c.c_str());
	createStatusChannel();
}

void Account::enqueueChannelJoin(const string& c)
{
	assert(isValid());
	string list = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "join_queue", "");
	if(!list.empty())
		list += ",";
	list += c;
	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "join_queue", list.c_str());
}

void Account::flushChannelJoins()
{
	assert(isValid());
	string list = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "join_queue", "");
	string cname;
	while((cname = stringtok(list, ",")).empty() == false)
		this->joinChat(cname, "");
	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "join_queue", "");
}

void Account::abortChannelJoins()
{
	assert(isValid());

	irc::IRC* irc = Purple::getIM()->getIRC();
	string list = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "join_queue", "");
	string cname;

	while((cname = stringtok(list, ",")).empty() == false)
		irc->getUser()->send(irc::Message(ERR_NOSUCHCHANNEL).setSender(irc)
		                                                    .setReceiver(irc->getUser())
		                                                    .addArg("#" + cname + ":" + getID())
		                                                    .addArg("No such channel"));

	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "join_queue", "");

}

string Account::getServername() const
{
	assert(isValid());
	return getUsername() + ":" + getID();
}

PurpleConnection* Account::getPurpleConnection() const
{
	assert(isValid());
	return purple_account_get_connection(account);
}

bool Account::isConnected() const
{
	assert(isValid());
	return purple_account_is_connected(account);
}

bool Account::isConnecting() const
{
	assert(isValid());
	return purple_account_is_connecting(account);
}

vector<Buddy> Account::getBuddies() const
{
	assert(isValid());
	vector<Buddy> buddies;
	if(!purple_get_blist())
		return buddies;

	GSList* bl = purple_find_buddies(account, NULL);
	GSList* cur;

	for(cur = bl; cur != NULL; cur = cur->next)
	{
		if(!PURPLE_BLIST_NODE_IS_BUDDY((PurpleBlistNode*)cur->data))
			continue;
		PurpleBuddy* b = (PurpleBuddy*)cur->data;
		buddies.push_back(Buddy(b));
	}
	g_slist_free(bl);

	return buddies;
}

void Account::updatedAllBuddies() const
{
	assert(isValid());
	vector<Buddy> buddies = getBuddies();
	for(vector<Buddy>::iterator it = buddies.begin(); it != buddies.end(); ++it)
		it->updated();
}

void Account::displayRoomList() const
{
	assert(isValid());
	purple_roomlist_get_list(account->gc);
}

void Account::connect() const
{
	assert(isValid());
	purple_account_set_enabled(account, MINBIF_VERSION_NAME, true);
}

void Account::disconnect() const
{
	assert(isValid());
	removeReconnection(true);
	purple_account_set_enabled(account, MINBIF_VERSION_NAME, false);
}

int Account::delayReconnect() const
{
	assert(isValid());
	int delay = purple_account_get_ui_int(account, MINBIF_VERSION_NAME, "delay-reconnect", 15);
	delay *= 2;

	int id = purple_account_get_ui_int(account, MINBIF_VERSION_NAME, "id-reconnect", -1);
	if(id >= 0)
		g_source_remove(id);
	id = g_timeout_add(delay*1000, Account::reconnect, account);

	purple_account_set_ui_int(account, MINBIF_VERSION_NAME, "delay-reconnect", delay);
	purple_account_set_ui_int(account, MINBIF_VERSION_NAME, "id-reconnect", id);
	return delay;
}

void Account::removeReconnection(bool verbose) const
{
	assert(isValid());
	int id = purple_account_get_ui_int(account, MINBIF_VERSION_NAME, "id-reconnect", -1);
	if(id >= 0)
	{
		g_source_remove(id);
		if(verbose)
			b_log[W_INFO|W_SNO] << "Abort auto-reconnection to " << getServername();
	}

	purple_account_set_ui_int(account, MINBIF_VERSION_NAME, "delay-reconnect", 15);
	purple_account_set_ui_int(account, MINBIF_VERSION_NAME, "id-reconnect", -1);
}

irc::StatusChannel* Account::getStatusChannel() const
{
	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::StatusChannel* chan;
	string channame = getStatusChannelName();

	if(!irc::Channel::isStatusChannel(channame))
		return NULL;

	chan = dynamic_cast<irc::StatusChannel*>(irc->getChannel(channame));
	if(!chan)
	{
		b_log[W_ERR] << "Status channel " << channame << " is not found.";
		return NULL;
	}

	return chan;
}

void Account::createStatusChannel()
{
	assert(isValid());

	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::StatusChannel* chan;
	string channame = getStatusChannelName();

	if(!irc::Channel::isStatusChannel(channame))
		return;

	chan = dynamic_cast<irc::StatusChannel*>(irc->getChannel(channame));
	if(!chan)
	{
		chan = new irc::StatusChannel(irc, channame);
		irc->addChannel(chan);
		irc->getUser()->join(chan, irc::ChanUser::OP);
	}

	string status = getStatusMessage(PURPLE_STATUS_AVAILABLE);
	b_log[W_ERR] << getID() << ": " << status;
	if (chan->getTopic() != status)
	{
		if (!status.empty())
			chan->setTopic(NULL, status);
		else
			setStatus(PURPLE_STATUS_UNSET, chan->getTopic());
	}

	chan->addAccount(*this);

	vector<Buddy> buddies = getBuddies();
	for(vector<Buddy>::iterator b = buddies.begin(); b != buddies.end(); ++b)
		b->updated();
}

void Account::leaveStatusChannel()
{
	assert(isValid());

	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::StatusChannel* chan = getStatusChannel();
	if (!chan)
		return;

	chan->removeAccount(*this);

	vector<Buddy> buddies = getBuddies();
	for(vector<Buddy>::iterator b = buddies.begin(); b != buddies.end(); ++b)
		if(b->getNick())
			b->getNick()->part(chan, "Leaving status channel");

	if(chan->countAccounts() == 0)
		irc->removeChannel(chan->getName());
}

vector<string> Account::getDenyList() const
{
	assert(isValid());

	vector<string> list;
	GSList *l;
	for (l = account->deny; l != NULL; l = l->next)
		list.push_back((const char*)l->data);
	return list;
}

bool Account::deny(const string& who) const
{
	assert(isValid());

	if (account->perm_deny != PURPLE_PRIVACY_DENY_USERS)
	{
		account->perm_deny = PURPLE_PRIVACY_DENY_USERS;
		if (purple_account_is_connected(account))
			serv_set_permit_deny(purple_account_get_connection(account));
	}

	return purple_privacy_deny_add(account, who.c_str(), FALSE);
}

bool Account::allow(const string& who) const
{
	assert(isValid());

	return purple_privacy_deny_remove(account, who.c_str(), FALSE);
}

void Account::addBuddy(const string& username, const string& group) const
{
	assert(isValid());
	assert(username.empty() == false);
	assert(group.empty() == false);

	PurpleGroup* grp = purple_find_group(group.c_str());
	if (!grp)
	{
		grp = purple_group_new(group.c_str());
		purple_blist_add_group(grp, NULL);
	}

	PurpleBuddy* buddy = purple_buddy_new(account, username.c_str(), username.c_str());
	purple_blist_add_buddy(buddy, NULL, grp, NULL);
	purple_account_add_buddy(account, buddy);
}

void Account::removeBuddy(Buddy buddy) const
{
	assert(isValid());

	purple_account_remove_buddy(account, buddy.getPurpleBuddy(), buddy.getPurpleGroup());
	purple_blist_remove_buddy(buddy.getPurpleBuddy());
}

bool Account::supportsChats() const
{
	assert(isValid());
	PurpleConnection *gc = purple_account_get_connection(account);
	PurplePluginProtocolInfo *prpl_info = NULL;

	prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl);

	return (prpl_info->chat_info != NULL);
}

map<string, string> Account::getChatParameters() const
{
	assert(isValid());
	assert(isConnected());

	PurpleConnection* gc = purple_account_get_connection(account);
	map<string, string> m;
	GList *list = NULL, *tmp;
	GHashTable *defaults = NULL;

	if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info != NULL)
		list = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info(gc);
	if (PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults != NULL)
		defaults = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl)->chat_info_defaults(gc, NULL);

	for(tmp = list; tmp; tmp = tmp->next)
	{
		struct proto_chat_entry *pce = static_cast<struct proto_chat_entry*>(tmp->data);
		char* value = (char*)g_hash_table_lookup(defaults, pce->identifier);
		m[pce->identifier] = value ? value : "";
		g_free(pce);
	}

	g_list_free(list);
	g_hash_table_destroy(defaults);
	return m;
}

bool Account::joinChat(const string& name, const string& parameters) const
{
	assert(isValid());
	if(!isConnected())
	{
		b_log[W_SNO|W_INFO] << "Not connected";
		return false;
	}

#if 0
	PurpleConnection* gc = purple_account_get_connection(account);
	PurpleConversation* conv;
	if (!(conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT, name.c_str(), account)))
	{
		conv = purple_conversation_new(PURPLE_CONV_TYPE_CHAT, account, name.c_str());
		purple_conv_chat_left(PURPLE_CONV_CHAT(conv));
	}
	else
		purple_conversation_present(conv);

	PurpleChat *chat;
	GHashTable *hash = NULL;

	chat = purple_blist_find_chat(account, name.c_str());
	if (chat == NULL)
	{
		PurplePluginProtocolInfo *info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_connection_get_prpl(gc));
		if (info->chat_info_defaults != NULL)
			hash = info->chat_info_defaults(gc, name.c_str());
	}
	else
		hash = purple_chat_get_components(chat);

	serv_join_chat(gc, hash);
	if (chat == NULL && hash != NULL)
		g_hash_table_destroy(hash);

#else /* !0 */
	if(!supportsChats())
	{
		/* Ok, so chats are not officially supported on this protocol...
		 * But there is another way to create a group chat, for example
		 * on MSN, where we can initiate a chat with a buddy, and invite
		 * others people in.
		 *
		 * But there is only one way to create this kind of group chat,
		 * it's to call the callback function in a buddy's menu from
		 * protocol plugin.
		 *
		 * So this FUCKING HACK is looking for the menu, and find the
		 * string "Initiate _Chat", and call his callback.
		 *
		 * It's yeah ugly.
		 */
		PurpleConnection *gc = purple_account_get_connection(account);
		GList *l, *ll;
		PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(gc->prpl);

		if(!prpl_info || !prpl_info->blist_node_menu)
			return false;

		irc::Buddy* n = dynamic_cast<irc::Buddy*>(Purple::getIM()->getIRC()->getNick(name));
		if(!n)
			return false;
		PurpleBlistNode* node = (PurpleBlistNode*)n->getBuddy().getPurpleBuddy();
		if(!node)
			return false;

		for(l = ll = prpl_info->blist_node_menu(node); l; l = l->next) {
			PurpleMenuAction *act = (PurpleMenuAction *) l->data;
			if(!act->children && act->callback != NULL && !strcasecmp(act->label, "Initiate _Chat"))
			{
				((void (*) (gpointer, gpointer))act->callback)(node, act->data);
				g_list_free(ll);
				return true;
			}
		}
		g_list_free(ll);
		return false;
	}

	PurpleChat *chat;
	GHashTable *hash = NULL;
	PurpleConnection *gc;
	PurplePluginProtocolInfo *info;

	gc = purple_account_get_connection(account);
	info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_connection_get_prpl(gc));
	if (info->chat_info_defaults != NULL)
		hash = info->chat_info_defaults(gc, name.c_str());

	/* Parse parameters */
	string param, params = parameters;
	while((param = stringtok(params, ";")).empty() == false)
	{
		string key = stringtok(param, "=");
		if(key.empty()) continue;
		g_hash_table_replace(hash,
				g_strdup(key.c_str()),
				g_strdup(param.c_str()));
	}

	chat = purple_chat_new(account, name.c_str(), hash);

	if (chat != NULL) {
		//if ((grp = purple_find_group(group)) == NULL) {
		//	grp = purple_group_new(group);
		//	purple_blist_add_group(grp, NULL);
		//}
		//purple_blist_add_chat(chat, grp, NULL);
		//purple_blist_alias_chat(chat, alias);
		//purple_blist_node_set_bool((PurpleBlistNode*)chat, "gnt-autojoin", autojoin);
		const char *name;
		PurpleConversation *conv;
		const char *alias;

		/* This hack here is to work around the fact that there's no good way of
		 * getting the actual name of a chat. I don't understand why we return
		 * the alias for a chat when all we want is the name. */
		alias = chat->alias;
		chat->alias = NULL;
		name = purple_chat_get_name(chat);
		conv = purple_find_conversation_with_account(
				PURPLE_CONV_TYPE_CHAT, name, account);
		chat->alias = (char *)alias;

		if (!conv || purple_conv_chat_has_left(PURPLE_CONV_CHAT(conv))) {
			serv_join_chat(purple_account_get_connection(account),
					purple_chat_get_components(chat));
		} else if (conv) {
			purple_conversation_present(conv);
		}
	}
#endif /* !0 */

	return chat != NULL;

}

/* STATIC */

PurpleConnectionUiOps Account::conn_ops =
{
	Account::connecting,
        Account::connected,
        Account::disconnected,
        NULL, /* notice */
        NULL,
        NULL, /* network_connected */
        NULL, /* network_disconnected */
        Account::disconnect_reason,
        NULL,
        NULL,
        NULL
};

PurpleAccountUiOps Account::acc_ops =
{
        notify_added,
        NULL,
        request_add,
        request_authorize,
        request_close,
        NULL,
        NULL,
        NULL,
        NULL
};


void* Account::getHandler()
{
	static int handler;

	return &handler;
}

void Account::init()
{
	purple_connections_set_ui_ops(&conn_ops);
	purple_accounts_set_ui_ops(&acc_ops);
	purple_signal_connect(purple_accounts_get_handle(), "account-added",
				getHandler(), PURPLE_CALLBACK(account_added),
				NULL);
	purple_signal_connect(purple_accounts_get_handle(), "account-removed",
				getHandler(), PURPLE_CALLBACK(account_removed),
				NULL);
	purple_signal_connect(purple_connections_get_handle(), "signed-on", getHandler(),
				G_CALLBACK(account_signed_on_cb),
				GINT_TO_POINTER(PURPLE_CONV_ACCOUNT_ONLINE));


	map<string, Account> accounts = Purple::getAccountsList();
	for(map<string, Account>::iterator it = accounts.begin(); it != accounts.end(); ++it)
		it->second.createStatusChannel();
}

void Account::uninit()
{
	purple_accounts_set_ui_ops(NULL);
	purple_connections_set_ui_ops(NULL);
	purple_signals_disconnect_by_handle(getHandler());
}

gboolean Account::reconnect(void* data)
{
	Account acc((PurpleAccount*)data);
	purple_account_set_ui_int(acc.account, MINBIF_VERSION_NAME, "id-reconnect", -1);
	acc.connect();
	return FALSE;
}

void Account::account_added(PurpleAccount* account)
{
}

void Account::account_removed(PurpleAccount* a)
{
	Account account(a);
	account.abortChannelJoins();
	account.removeReconnection();
	account.leaveStatusChannel();
}

static char *make_info(PurpleAccount *account, PurpleConnection *gc, const char *remote_user,
		  const char *id, const char *alias, const char *msg)
{
	if (msg != NULL && *msg == '\0')
		msg = NULL;

	return g_strdup_printf("%s%s%s%s has made %s his or her buddy%s%s",
			remote_user,
			(alias != NULL ? " ("  : ""),
			(alias != NULL ? alias : ""),
			(alias != NULL ? ")"   : ""),
			(id != NULL
				? id
				: (purple_connection_get_display_name(gc) != NULL
				   ? purple_connection_get_display_name(gc)
				   : purple_account_get_username(account))),
			(msg != NULL ? ": " : "."),
			(msg != NULL ? msg  : ""));
}

typedef struct
{
	PurpleAccount *account;
	char *username;
	char *alias;
} AddUserData;

static void
free_add_user_data(AddUserData *data)
{
	g_free(data->username);

	if (data->alias != NULL)
		g_free(data->alias);

	g_free(data);
}

static void
add_user_cb(AddUserData *data)
{
	PurpleConnection *gc = purple_account_get_connection(data->account);

	if (g_list_find(purple_connections_get_all(), gc))
	{
		purple_blist_request_add_buddy(data->account, data->username,
									 NULL, data->alias);
	}

	free_add_user_data(data);
}

void Account::notify_added(PurpleAccount *account, const char *remote_user,
				const char *id, const char *alias,
				const char *msg)
{
	char *buffer;
	PurpleConnection *gc;

	gc = purple_account_get_connection(account);

	buffer = make_info(account, gc, remote_user, id, alias, msg);

	purple_notify_info(NULL, NULL, buffer, NULL);

	g_free(buffer);
}

void Account::request_add(PurpleAccount *account, const char *remote_user,
			  const char *id, const char *alias,
			  const char *msg)
{
	char *buffer;
	PurpleConnection *gc;
	AddUserData *data;

	gc = purple_account_get_connection(account);

	data = g_new0(AddUserData, 1);
	data->account  = account;
	data->username = g_strdup(remote_user);
	data->alias    = (alias != NULL ? g_strdup(alias) : NULL);

	buffer = make_info(account, gc, remote_user, id, alias, msg);
	purple_request_action(NULL, NULL, "Add buddy to your list?",
	                    buffer, PURPLE_DEFAULT_ACTION_NONE,
						account, remote_user, NULL,
						data, 2,
	                    "Add",    G_CALLBACK(add_user_cb),
	                    "Cancel", G_CALLBACK(free_add_user_data));
	g_free(buffer);

}

typedef struct {
	PurpleAccountRequestAuthorizationCb auth_cb;
	PurpleAccountRequestAuthorizationCb deny_cb;
	void *data;
	char *username;
	char *alias;
	PurpleAccount *account;
} auth_and_add;

static void
free_auth_and_add(auth_and_add *aa)
{
	g_free(aa->username);
	g_free(aa->alias);
	g_free(aa);
}

static void
authorize_and_add_cb(auth_and_add *aa)
{
	aa->auth_cb(aa->data);
	purple_blist_request_add_buddy(aa->account, aa->username,
		                       NULL, aa->alias);
	free_auth_and_add(aa);
}

static void
deny_no_add_cb(auth_and_add *aa)
{
	aa->deny_cb(aa->data);
	free_auth_and_add(aa);
}

void *Account::request_authorize(PurpleAccount *account,
				const char *remote_user,
				const char *id,
				const char *alias,
				const char *message,
				gboolean on_list,
				PurpleAccountRequestAuthorizationCb auth_cb,
				PurpleAccountRequestAuthorizationCb deny_cb,
				void *user_data)
{
	char *buffer;
	PurpleConnection *gc;
	void *uihandle;

	gc = purple_account_get_connection(account);
	if (message != NULL && *message == '\0')
		message = NULL;

	buffer = g_strdup_printf("%s%s%s%s wants to add %s to his or her buddy list%s%s",
				remote_user,
		                (alias != NULL ? " ("  : ""),
		                (alias != NULL ? alias : ""),
		                (alias != NULL ? ")"   : ""),
		                (id != NULL
		                ? id
		                : (purple_connection_get_display_name(gc) != NULL
		                ? purple_connection_get_display_name(gc)
		                : purple_account_get_username(account))),
		                (message != NULL ? ": " : "."),
		                (message != NULL ? message  : ""));
	if (!on_list) {
		auth_and_add *aa = g_new(auth_and_add, 1);

		aa->auth_cb = auth_cb;
		aa->deny_cb = deny_cb;
		aa->data = user_data;
		aa->username = g_strdup(remote_user);
		aa->alias = g_strdup(alias);
		aa->account = account;

		uihandle = purple_request_action(NULL, "Authorize buddy?", buffer, NULL,
			PURPLE_DEFAULT_ACTION_NONE,
			account, remote_user, NULL,
			aa, 2,
			"Authorize", authorize_and_add_cb,
			"Deny", deny_no_add_cb);
	} else {
		uihandle = purple_request_action(NULL, "Authorize buddy?", buffer, NULL,
			PURPLE_DEFAULT_ACTION_NONE,
			account, remote_user, NULL,
			user_data, 2,
			"Authorize", auth_cb,
			"Deny", deny_cb);
	}
	g_free(buffer);
	return uihandle;
}

void Account::request_close(void *uihandle)
{
	purple_request_close(PURPLE_REQUEST_ACTION, uihandle);
}

void Account::connecting(PurpleConnection *gc,
		const char *text,
		size_t step,
		size_t step_count)
{
	Account account = Account(gc->account);

	if(!step)
		b_log[W_INFO|W_SNO] << "Connection to " << account.getServername() << " in progress...";
	else
		b_log[W_INFO|W_SNO] << "" << account.getID() << "(" << step << "/" << step_count-1 << "): " << text;
}

void Account::connected(PurpleConnection* gc)
{
	Account account = Account(gc->account);
	account.removeReconnection();
	irc::IRC* irc = Purple::getIM()->getIRC();

	b_log[W_INFO|W_SNO] << "Connection to " << account.getServername() << " established!";
	irc->addServer(new irc::RemoteServer(irc, account));
	account.flushChannelJoins();
}

void Account::account_signed_on_cb(PurpleConnection *gc, gpointer event)
{
	Account account = Account(gc->account);
	GList* list = purple_get_chats();

	/* Rejoin channels. */
	for(; list; list = list->next)
	{
		PurpleChat *chat;
		GHashTable *comps = NULL;
		PurpleConversation *conv = (PurpleConversation*)list->data;
		Conversation c(conv);

		if(c.getAccount() != account ||
		   c.getType() != PURPLE_CONV_TYPE_CHAT ||
		   !purple_conversation_get_data(conv, "want-to-rejoin"))
			continue;
		chat = purple_blist_find_chat(purple_conversation_get_account(conv), purple_conversation_get_name(conv));

		if (chat == NULL) {
			PurplePluginProtocolInfo *info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_connection_get_prpl(gc));
			if (info->chat_info_defaults != NULL)
				comps = info->chat_info_defaults(gc, purple_conversation_get_name(conv));
		} else {
			comps = purple_chat_get_components(chat);
		}
		serv_join_chat(gc, comps);
		if (chat == NULL && comps != NULL)
			g_hash_table_destroy(comps);

	}

}

void Account::disconnected(PurpleConnection* gc)
{
	Account account = Account(gc->account);
	GList* list = purple_get_chats();

	account.abortChannelJoins();

	/* Enqueue channels to auto-rejoin. */
	for(; list; list = list->next)
	{
		Conversation c((PurpleConversation*)list->data);
		if(c.getAccount() != account)
			continue;
		if(purple_conv_chat_has_left(c.getPurpleChat()))
		{
			string name = c.getName();
			c.leave();
			account.enqueueChannelJoin(name);
		}
		else
			purple_conversation_set_data(c.getPurpleConversation(), "want-to-rejoin", GINT_TO_POINTER(TRUE));
	}

	b_log[W_INFO|W_SNO] << "Closing link with " << account.getServername();
	Purple::getIM()->getIRC()->removeServer(account.getServername());
}

void Account::disconnect_reason(PurpleConnection *gc,
				PurpleConnectionError reason,
				const char *text)
{
	Account acc(gc->account);
	b_log[W_ERR|W_SNO] << "Error(" << acc.getServername() << "): " << text;
	switch(reason)
	{
	case PURPLE_CONNECTION_ERROR_NETWORK_ERROR:
	case PURPLE_CONNECTION_ERROR_ENCRYPTION_ERROR:
	case PURPLE_CONNECTION_ERROR_OTHER_ERROR:
		b_log[W_ERR|W_SNO] << "Reconnection in " << acc.delayReconnect() << " seconds";
		break;
	default:
		break; /* Do not auto reconnect. */
	}
}

}; /* namespace im */
