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

#include "account.h"
#include "buddy.h"
#include "purple.h"
#include "../log.h"
#include "../version.h"
#include "im.h"
#include "irc/irc.h"
#include "irc/status_channel.h"
#include "irc/user.h"

namespace im {

Account::Account()
	: account(NULL)
{}

Account::Account(PurpleAccount* _account, Protocol _proto)
	: account(_account),
	  proto(_proto)
{
	if(!proto.isValid())
		proto = Purple::getProtocolByPurpleID(account->protocol_id);
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

string Account::getID() const
{
	assert(isValid());
	string n = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "id", "");
	if(n.empty())
		n = Purple::getNewAccountName(proto);
	return n;
}

string Account::getStatusChannel() const
{
	assert(isValid());
	string n = purple_account_get_ui_string(account, MINBIF_VERSION_NAME, "channel", "");
	return n;
}

void Account::setStatusChannel(const string& c)
{
	assert(isValid());
	purple_account_set_ui_string(account, MINBIF_VERSION_NAME, "channel", c.c_str());
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
	return buddies;
}

void Account::connect() const
{
	assert(isValid());
	purple_account_set_enabled(account, MINBIF_VERSION_NAME, true);
}

void Account::disconnect() const
{
	assert(isValid());
	purple_account_set_enabled(account, MINBIF_VERSION_NAME, false);
}

void Account::createStatusChannel() const
{
	assert(isValid());

	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::StatusChannel* chan;
	string channame = getStatusChannel();

	if(!irc::Channel::isStatusChannel(channame))
		return;

	chan = dynamic_cast<irc::StatusChannel*>(irc->getChannel(channame));
	if(!chan)
	{
		chan = new irc::StatusChannel(irc, channame);
		irc->addChannel(chan);
		irc->getUser()->join(chan);
	}
	chan->addAccount(*this);
}

void Account::leaveStatusChannel() const
{
	assert(isValid());

	irc::IRC* irc = Purple::getIM()->getIRC();
	irc::StatusChannel* chan;
	string channame = getStatusChannel();

	if(!irc::Channel::isStatusChannel(channame))
		return;

	chan = dynamic_cast<irc::StatusChannel*>(irc->getChannel(channame));
	if(chan)
		chan->removeAccount(*this);
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

void Account::deny(const string& who) const
{
	assert(isValid());

	purple_privacy_deny(account, who.c_str(), FALSE, FALSE);
}

void Account::allow(const string& who) const
{
	assert(isValid());

	purple_privacy_allow(account, who.c_str(), FALSE, FALSE);
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

bool Account::joinChat(const string& name) const
{
	assert(isValid());
	if(!isConnected())
	{
		b_log[W_SNO] << "Not connected";
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
	PurpleChat *chat;
	GHashTable *hash = NULL;
	PurpleConnection *gc;
	PurplePluginProtocolInfo *info;


	gc = purple_account_get_connection(account);
	info = PURPLE_PLUGIN_PROTOCOL_INFO(purple_connection_get_prpl(gc));
	if (info->chat_info_defaults != NULL)
		hash = info->chat_info_defaults(gc, name.c_str());

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
        NULL,//notify_added,
        NULL,
        NULL,//request_add,
        NULL,//finch_request_authorize,
        NULL,//finch_request_close,
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

	map<string, Account> accounts = Purple::getAccountsList();
	for(map<string, Account>::iterator it = accounts.begin(); it != accounts.end(); ++it)
		it->second.createStatusChannel();
}

void Account::account_added(PurpleAccount* account)
{
}

void Account::account_removed(PurpleAccount* a)
{
	Account account(a);
	account.leaveStatusChannel();
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
	irc::IRC* irc = Purple::getIM()->getIRC();

	b_log[W_INFO|W_SNO] << "Connection to " << account.getServername() << " established!";
	irc->addServer(new irc::RemoteServer(irc, account));
}

void Account::disconnected(PurpleConnection* gc)
{
	Account account = Account(gc->account);

	b_log[W_INFO|W_SNO] << "Closing link with " << account.getServername();
	Purple::getIM()->getIRC()->removeServer(account.getServername());
}

void Account::disconnect_reason(PurpleConnection *gc,
				PurpleConnectionError reason,
				const char *text)
{
	b_log[W_ERR|W_SNO] << "Error(" << Account(gc->account).getServername() << "): " << text;
}

}; /* namespace im */
