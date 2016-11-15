#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  asterisk.py
#
#  Copyright 2014 James Finstrom<jfinstrom at gmail>
#  Updated & Fixed Sep 2016 Jeff T - github jefft4
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
## note:  give the Manager user READ, WRITE on system and command (needed to run SIP commands) + read on whatever else it needs for other purposes.

import os
import sys
import socket
import ConfigParser

def make_dict(lst):
    ret = {}
    for i in lst:
        i = i.strip()
        if i and i[0] is not "#" and i[-1] is not "=":
            var, val = i.split(":", 1)
            ret[var.strip()] = val.strip()
    return ret


def verify_items(items, args):
    if items == None or args == None:
        return False
    
    items_dict = make_dict(items)
    
    for key, value in args.items():
        if key not in items_dict:
            print("The key is not in items. key[%s]" % (key))
            return False
        
        if value == None:
            continue
        
        if items_dict[key] != value:
            print("The key and value is not match. key[%s], value[%s]" % (key, value))
            return False
    
    return True


def verify_items_(items, args):
    size = len(items)
    for i in range(size):
        res_dict = make_dict(items[i])
        flg = True
        for key, value in args.items():
            if key not in res_dict:
                flg = False
                continue
            if value == None:
                continue
            if res_dict[key] != value:
                flg = False
                continue
        
        if flg == True:
            break
    
    return flg

class acli:
    def __init__(self, ip="127.0.0.1", port=5038, filename="/etc/asterisk/manager.conf", username="admin"):        
        config = ConfigParser.ConfigParser()
        config.read(filename)
        
        self.username = username
        self.password = config.get(username, 'secret')
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serverip = ip
        self.serverport = port

    def recvRes(self):
        data = []
        while '\r\n\r\n' not in ''.join(data)[-4:]:
            buf = self.sock.recv(1)
            data.append(buf)
        res = ''.join(data).split('\r\n')

        # Remove empty objects.
        res.remove('')
        res.remove('')

        return res

    def sendCmd(self, action, **args):
        self.sock.send("Action: %s\r\n" % action)
        for key, value in args.items():
            self.sock.send("%s: %s\r\n" % (key,value))
            
        self.sock.send("\r\n")
        return self.recvRes()

    def recvArr(self):
        res = []
        while True:
            tmp = self.recvRes()
            res.append(tmp)
            if 'EventList: Complete' in ''.join(tmp):
                break
        return res

    def conn(self):
        self.sock.connect((self.serverip, self.serverport))
        #need Events OFF with the login else event text pollutes our command response
        ret = self.sendCmd("login", Username=self.username, Secret=self.password, Events="OFF")
        #print "Connect response: ", ret
        if 'Response: Success' in ret:
            return True
        else:
            print "Connect failed!"
            return False


def main():
        ast = acli()
        ast.username = username
        ast.password = password
        if ast.conn():
                res = ast.sendCmd('QueueStatus')
                print res
                res = ast.recvArr()
                print res
                #dev = ast.sendCmd('SIPShowPeer', Peer='myvoiptrunkname')
                dev = ast.sendCmd('SIPpeers')
                #print "Command response: ", dev
                value = make_dict(dev)
                if value['Response'] == 'Success':
                        res = ast.recvArr()
                        for i in res:
                                print i
                        return
                        #print "Status = #", value['Status'], "#"
                        #don't test only for "OK" here, some return longer strings with ping time etc
                        if 'OK' in value['Status']:
                                print 'OK: trunk is up.'
                                pass
                        else:
                                callCavalry(value['Status'], 'peer myvoiptrunkname')
                else:
                        callCavalry(value['Message'], 'api call')

        return 0

if __name__ == '__main__':
        main()

