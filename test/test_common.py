# test_common.py
#  Created on: Dec 2, 2015
#      Author: pchero

import socket
import uuid


def make_dict(lst):
    ret ={}
    for i in lst:
        i = i.strip()
        if i and i[0] is not "#" and i[-1] is not "=":
            try:
                var,val = i.split(":", 1)
            except:
                continue
            ret[var.strip()] = val.strip()
    return ret

class Ami:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serverip = '127.0.0.1'
        self.serverport = 5038
        self.username = ''
        self.password = ''
    
    def sendCmd(self, action, **args):
        # send command.
        action_id = uuid.uuid4().__str__()
        self.sock.send("Action: %s\r\n" % action)
        self.sock.send("ActionID: %s\r\n" % (action_id))
        if args != None:
            for key, value in args.items():
                self.sock.send("%s: %s\r\n" % (key,value))
        self.sock.send("\r\n")
        
        # get result.
        c = 0
        recvs = []
        list = False
        while(c < 5):
            c += 1
            data = ""
            while '\r\n\r\n' not in ''.join(data)[-4:]:
                buf = self.sock.recv(1)
                data += buf
            recv = data.split('\r\n')
            tmp = make_dict(recv)
            
            if "ActionID" not in tmp:
                continue
            if tmp["ActionID"] != action_id:
                continue
            
            recvs.append(tmp)
            
            if list == False:
                if tmp["Message"].find("follow") > 0:
                    list = True
                    continue
            
            if list == True:
                if tmp["Event"].find("Complete") > 0:
                    break            
            if list == False:
                break
        return recvs
    
    def __sendCmd(self, action, **args):
        # send command.
        action_id = uuid.uuid4().__str__()
        self.sock.send("Action: %s\r\n" % action)
        self.sock.send("ActionID: %s\r\n" % (action_id))
        if args != None:
            for key, value in args.items():
                self.sock.send("%s: %s\r\n" % (key,value))
        self.sock.send("\r\n")
        
        # get result.
        c = 0
        recvs = []
        list = False
        while(c < 5):
            c += 1
            data = []
            while '\r\n\r\n' not in ''.join(data)[-4:]:
                buf = self.sock.recv(1)
                data.append(buf)
            recv = ''.join(data).split('\r\n')
            tmp = make_dict(recv)
            
            if "ActionID" not in tmp:
                continue
            if tmp["ActionID"] != action_id:
                continue
            
            recvs.append(tmp)
            
            if list == False:
                if tmp["Message"].find("follow") > 0:
                    list = True
                    continue
            
            if list == True:
                if tmp["Event"].find("Complete") > 0:
                    break            
            if list == False:
                break
        return recvs
    
    def recvEvt(self):
        data = []
        while '\r\n\r\n' not in ''.join(data)[-4:]:
            buf = self.sock.recv(1)
            data.append(buf)                
        recv = ''.join(data).split('\r\n')
        tmp = make_dict(recv)
        return tmp
                
    def conn(self):
        self.sock.connect((self.serverip, self.serverport))
        ret = self.sendCmd("login", Username=self.username, Secret=self.password)
        if ret[0]["Response"] != "Success":
            return False
        return True
