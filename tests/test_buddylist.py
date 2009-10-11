# -*- coding: utf-8 -*-

"""
Minbif - IRC instant messaging gateway
Copyright(C) 2009 Romain Bignon

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
"""

import sys
import re

from test import Test, Instance

class TestBuddyList(Test):
    NAME = 'buddy_list'
    INSTANCES = {'minbif1': Instance(), 'minbif2': Instance()}
    TESTS = ['init', 'addbuddy', 'renamebuddy', 'delbuddy']

    def test_init(self):
        if not self['minbif1'].create_account('jabber', channel='&minbif'): return False
        if not self['minbif1'].wait_connected('jabber0'): return False
        if not self['minbif1'].clean_buddies(): return False
        if not self['minbif2'].create_account('jabber', channel='&minbif'): return False
        if not self['minbif2'].wait_connected('jabber0'): return False
        if not self['minbif2'].clean_buddies(): return False

        return True

    def test_addbuddy(self):
        self['minbif1'].request_answer('New request:', 'authorize', 1)
        acc1name, acc1 = self['minbif1'].get_accounts().popitem()
        self['minbif2'].write('INVITE %s:jabber0 &minbif' % acc1.username)
        self['minbif1'].request_answer('New request:', 'authorize', 5)

        self['minbif2'].log('Wait for join')
        while 1:
            msg = self['minbif2'].readmsg('JOIN', 4)
            if not msg:
                return False

            m = re.match('([^!]*)!([^@]*)@(.*)', msg.sender)
            if m:
                return m.group(2) == acc1.username.split('@')[0]

    def test_renamebuddy(self):
        buddies = self['minbif2'].get_buddies()
        if len(buddies) < 1:
            return False

        nick, buddy = buddies.popitem()
        self['minbif2'].write('SVSNICK %s cacaprout' % nick)
        while 1:
            msg = self['minbif2'].readmsg('NICK', 2)
            if not msg:
                return False

            if msg.sender.startswith('%s!' % nick) and msg.receiver == 'cacaprout':
                return True

    def test_delbuddy(self):
        buddies = self['minbif2'].get_buddies()
        if len(buddies) < 1:
            self['minbif2'].log('Assert failed "len(buddies) == 1"')
            return False

        nick, buddy = buddies.popitem()
        self['minbif2'].write('KILL %s' % nick)
        self['minbif2'].log("Wait for quit")
        while 1:
            msg = self['minbif2'].readmsg('QUIT', 2)
            if not msg:
                return False

            return msg.sender.startswith('%s!' % nick)

if __name__ == '__main__':
    test = TestBuddyList()
    if test.run():
        sys.exit(0)
    else:
        sys.exit(1)
