# test_campaign.py
#  Created on: Nov 27, 2015
#      Author: pchero
 
import os
import sys
import socket
import uuid

def make_dict(lst):
    ret ={}
    for i in lst:
        i = i.strip()
        if i and i[0] is not "#" and i[-1] is not "=":
            try:
                var,val = i.rsplit(":",1)
            except:
                continue
            ret[var.strip()] = val.strip()
    return ret

class acli:
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
        for key, value in args.items():
            self.sock.send("%s: %s\r\n" % (key,value))
        self.sock.send("\r\n")
        
        # get result.
        c = 0
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
            break
        return tmp
    
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
        if ret["Response"] != "Success":
            return False
        return True
    
            
def main():
    ast = acli()
    ast.username = sys.argv[1]
    ast.password = sys.argv[2]
    if ast.conn() == False:
        print("Could not connect.")
        return 1
    
    # create campaign
    ret = ast.sendCmd("OutCampaignCreate", Name="TestCamp", Detail="TestDetail", Plan="5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56", Dlma="e276d8be-a558-4546-948a-f99913a7fea2", Queue="1c8eeabb-1dbc-4b75-a688-dd5b79b5afc6")
    if ret["Response"] != "Success":
        return 1
    while(1):
        ret = ast.recvEvt()
        if ret["Event"] == "OutCampaignCreate":
            break
        
    
    ret = ast.sendCmd("OutCampaignDelete", Uuid=ret["Uuid"])
    if ret["Response"] != "Success":
        print "Could not pass Campaign Delete."
        print ret
        return 1
    
                    
    return 0

if __name__ == '__main__':
    main()
