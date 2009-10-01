#!/bin/python

import sys
from test import Test, Instance

class TestAccounts(Test):
    NAME = 'accounts'
    INSTANCES = {'minbif1': Instance()}
    TESTS = ['addaccount']

    def test_addaccount(self):
        if not self['minbif1'].create_account('jabber', channel='&minbif'):
            return False

        accounts = self['minbif1'].get_accounts()
        if len(accounts) == 1:
            acc = accounts.popitem()[1]
            return acc.proto == 'jabber'
        return False

if __name__ == '__main__':
    test = TestAccounts()
    if test.run():
        sys.exit(0)
    else:
        sys.exit(1)
