#!/bin/python
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

class TestAccounts(Test):
    NAME = 'accounts'
    INSTANCES = {'minbif1': Instance()}
    TESTS = ['addaccount', 'connected', 'editaccount', 'disconnect', 'removeaccount']

    def test_addaccount(self):
        if not self['minbif1'].create_account('jabber', channel='&minbif'):
            return False

        accounts = self['minbif1'].get_accounts()
        if len(accounts) == 1:
            acc = accounts.popitem()[1]
            return acc.proto == 'jabber'
        return False

    def test_connected(self):
        return self['minbif1'].wait_connected('jabber0')

    def test_editaccount(self):
        # flush
        while self['minbif1'].readline():
            pass

        acc = self['minbif1'].get_full_account('jabber0')
        require_tls = acc.options['require_tls']

        self['minbif1'].write("MAP edit jabber0 require_tls %s" % (require_tls == "true" and "false" or "true"))
        self['minbif1'].readmsg('NOTICE', 1)

        acc = self['minbif1'].get_full_account('jabber0')
        return acc.options['require_tls'] == (require_tls == "true" and "false" or "true")

    def test_disconnect(self):
        self['minbif1'].write('SQUIT jabber0')
        accounts = self['minbif1'].get_accounts()
        return accounts.popitem()[1].state == 'disconnected'

    def test_removeaccount(self):
        self['minbif1'].remove_account('jabber0')
        accounts = self['minbif1'].get_accounts()
        return len(accounts) == 0

if __name__ == '__main__':
    test = TestAccounts()
    if test.run():
        sys.exit(0)
    else:
        sys.exit(1)
