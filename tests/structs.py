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

class Account:
    def __init__(self, proto='', username='', password='', options={}):
        self.proto = proto
        self.username = username
        self.password = password
        self.options = options
        self.state = ''

class Buddy:
    def __init__(self, servername, nickname='', username='', hostname='', realname=''):
        self.servername = servername
        self.nickname = nickname
        self.username = username
        self.hostname = hostname
        self.realname = realname
