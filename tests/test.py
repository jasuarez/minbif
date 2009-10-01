from __future__ import with_statement
import sys
import os
from select import select
from subprocess import Popen, PIPE, STDOUT
from time import sleep, time
import re

import config
from account import Account

NOBUFFER_PATH = os.path.normpath(os.path.join(os.path.dirname(__file__), 'libnobuffer.so'))
MINBIF_PATH = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', 'build.minbif', 'minbif'))

class Test:
    NAME = ''
    INSTANCES = {}
    TESTS = []
    PATH = '/tmp/minbif-tests'

    def __getitem__(self, inst):
        return self.INSTANCES[inst]

    def run(self, stop_on_failure=True):
        print 'Starting test: %s' % self.NAME

        ret = self._run(stop_on_failure)
        for name, instance in self.INSTANCES.iteritems():
            instance.stop()

        if ret:
            print 'Ending test %s: success' % self.NAME
        else:
            print 'Ending test %s: failed' % self.NAME
            self.display_logs()
        return ret

    def _run(self, stop_on_failure=True):
        if not self.rm_and_mkdir(self.PATH):
            return False

        for name, instance in self.INSTANCES.iteritems():
            sys.stdout.write('\tLaunch minbif %s: ' % name)

            inst_path = '%s/%s' % (self.PATH, name)
            if not self.rm_and_mkdir(inst_path):
                return False

            if instance.start(inst_path):
                print 'success'
            else:
                print 'failed'
                return False

        for test in self.TESTS:
            if not self.run_test(test) and stop_on_failure:
                return False

        return True

    def run_test(self, test):
        sys.stdout.write('\tTest %s: ' % test)
        if not hasattr(self, 'test_%s' % test):
            print '\t\t[Not found]'
        else:
            func = getattr(self, 'test_%s' % test)
            msg = ''
            try:
                ret = func()
            except Exception, e:
                ret = False
                msg = '%s: %s' % (type(e).__name__, str(e))
            if ret:
                print '\t\t[Success]'
                return True
            else:
                print '\t\t[Failed] %s' % msg

        return False

    def rm_and_mkdir(self, path):
        for root, dirs, files in os.walk(path, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))

        try:
            os.rmdir(path)
        except:
            pass

        try:
            os.mkdir(path)
        except OSError, e:
            print 'Error: unable to create directory %s: %s' % (path, e)
            return False

        return True

    def display_logs(self):
        for name, instance in self.INSTANCES.iteritems():
            print '\nLog for %s:' % name
            instance.display_logs()

class Message:
    def __init__(self, cmd, sender=None, receiver=None, args=[]):
        self.cmd = cmd
        self.sender = sender
        self.receiver = receiver
        self.args = args

    @staticmethod
    def parseline(line):
        args = line.split(' ')
        if not args:
            return None

        cmd = None
        sender = None
        receiver = None
        if args[0][0] == ':':
            sender = args.pop(0)[1:]
        cmd = args.pop(0)
        receiver = args.pop(0)
        for i in xrange(len(args)):
            if args[i][0] == ':':
                args[i] = ' '.join(args[i:])[1:]
                args = args[:i+1]
                break
        return Message(cmd, sender, receiver, args)

class Instance:
    DEFAULT_CONF = {'path': {'users': ''},
                    'irc':  {'hostname': 'im.symlink.me',
                             'type':  0,
                             'ping':  0,
                            },
                    'file_transfers': {'enabled': 'true',
                                       'dcc': 'true',
                                       'port_range': '1024-65535',
                                      },
                    'logging': {'level': 'DESYNCH WARNING ERR INFO DEBUG',
                                'to_syslog': 'false'
                               },
                   }
    def __init__(self, config={}):
        self.config = config
        self.path = ''
        self.logs = []
        self.process = None

    def display_logs(self):
        for log in self.logs:
            print '  %s' % log

    def log(self, s):
        self.logs.append('%.3f %s' % (time(), s))

    def stop(self):
        try:
            self.write("QUIT")
        except IOError:
            pass
        self.process = None

    def write(self, msg):
        if self.process:
            self.log("> %s" % msg)
            self.process.stdin.write("%s\n" % msg)

    def readline(self, timeout=0):
        out = self.process.stdout
        if timeout is not None:
            ready = select([out.fileno()], [], [], timeout)[0]
            if not ready:
                return None
        line = out.readline()
        if not line:
            return None
        line = line.rstrip()
        self.log("< %s" % line)
        return line

    def readmsg(self, cmd=None, timeout=0):
        while 1:
            line = self.readline(timeout)
            if not line:
                return None

            msg = Message.parseline(line)
            if not msg:
                print line
                continue

            if not cmd or isinstance(cmd, (str,unicode)) and msg.cmd == cmd \
                       or msg.cmd in cmd:
                return msg

    def start(self, path):
        try:
            self.path = path
            config_path = '%s/minbif.conf' % path
            self.write_config(config_path)
            self.process = Popen((MINBIF_PATH, config_path),
                                 stdin=PIPE, stdout=PIPE, stderr=STDOUT,
                                 env={"LD_PRELOAD": NOBUFFER_PATH})

            return self.login()
        except Exception, e:
            self.process = None
            sys.stdout.write("(%s) " % e)
            return False

    def update_config(self, config, config2):
        for key, value in config2.iteritems():
            if key in config and isinstance(config[key], dict):
                self.update_config(config[key], value)
            else:
                config[key] = value

    def write_config(self, filename):
        config = self.DEFAULT_CONF.copy()
        config['path']['users'] = '%s/users' % self.path
        self.update_config(config, self.config)
        self.config = config
        with open(filename, 'w') as f:
            self.write_config_section(f, 0, config)

    def write_config_section(self, fp, depth, section):
        tabs = '    ' * depth
        for key, value in section.iteritems():
            if isinstance(value, dict):
                fp.write("%s%s {\n" % (tabs, key))
                self.write_config_section(fp, depth+1, value)
                fp.write("%s}\n" % tabs)
            else:
                fp.write("%s%s = %s\n" % (tabs, key, value))

    def login(self, nickname="minbif", password="minbifrocks"):
        self.write("USER minbif * * :MinBif")
        self.write("PASS %s" % password)
        self.write("NICK %s" % nickname)

        msg = self.readmsg("001", 5)
        return (msg != None)

    def create_account(self, proto=None, channel='&minbif'):
        account = None
        if not proto:
            try:
                account = config.ACCOUNTS.pop()
            except IndexError:
                return False
        else:
            for acc in xrange(len(config.ACCOUNTS)):
                if config.ACCOUNTS[acc].proto == proto:
                    account = config.ACCOUNTS.pop(acc)
                    break

            if not account:
                return False

        options = ''
        for key, value in account.options:
            options += ' '
            if isinstance(value, bool):
                if value:
                    options += "-%s" % key
                else:
                    options += "-!%s" % key
            else:
                options += "-%s \"%s\"" % (key, value)

        self.write("MAP add %s %s %s %s %s" % (account.proto,
                                               account.username,
                                               account.password,
                                               options,
                                               channel))
        return True

    def get_accounts(self):
        # flush
        while self.readline():
            pass

        self.write("MAP")
        accounts = {}
        while 1:
            msg = self.readmsg(("015", "017"), 5)
            if not msg:
                return False

            if msg.cmd == "017":
                break

            line = msg.args[0]
            # me
            if not line.startswith('|-') and not line.startswith('`-'):
                continue

            m = re.match(".-(.+):([[a-zA-Z]+)([0-9]+)(\s*)(.*)", line)
            if m:
                acc = Account(proto=m.group(2), username=m.group(1))
                accounts['%s%s' % (m.group(2), m.group(3))] = acc

        return accounts
