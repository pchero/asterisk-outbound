# -*- coding: utf-8 -*-
"""
Created on Tue Dec 27 20:43:15 2016

@author: pchero
"""

import socket

class MainControl(object):
    sock = None
    connect = False
    buf = []
    
    username = None
    password = None
    ip = None
    port = None
    
    data_handler = None
    view_handler = None
    
    def __init__(self):
        print("MainControl init")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        return


    def set_data_handler(self, handler):
        self.data_handler = handler


    def set_veiw_handler(self, handler):
        self.view_handler = handler        
    
    
    def send_cmd_async(self, action, data=None):        
        if self.sock == None or action == None:
            return

        print("sendCmdAsync. action[%s], data[%s]" % (action, data))
        
        self.sock.send("Action: %s\r\n" % action)
        if data != None:
            for key, value in data.items():
                self.sock.send("%s: %s\r\n" % (key,value))
            
        self.sock.send("\r\n")
        return


    def recv_data(self):
        #print("recv_data")
        if self.connect == False:
            print("Not connected.")
            return False
        
        try:
            buf = self.sock.recv(1)
            #print("Received data. buf[%s]" % buf)
            if buf == None:
                return False
                
            self.buf.append(buf)
            self._message_handler()
        except socket.error:
            return False
        return True
        
        
    def _message_parser(self):
        if self.buf == None:
            return None

        # check correct message format        
        if "\r\n\r\n" != ''.join(self.buf)[-4:]:
            return None
        
        data = ''.join(self.buf).split('\r\n')
        self.buf = []
        
        # Remove empty objects.
        data.remove('')
        data.remove('')

        # create result        
        res = {}        
        try:
            for msg in data:
                tmp = msg.split(":", 1)
                if tmp == None or len(tmp) < 2:
                    continue
                res[tmp[0]] = tmp[1].strip()
        except Exception as e:
            print("Could not parse message. err[%s]" % e)
            return

        return res

    
    def login_handler(self, ip=None, port=None, username=None, password=None):
        print("loigin_handler. ip[%s], port[%s], username[%s], password[%s]" % (ip, port, username, password))

        # set values        
        self.username = username
        self.password = password
        self.ip = ip
        self.port = port
        print("Check value. ip[%s], port[%s], username[%s], password[%s]" % (self.ip, self.port, self.username, self.password))

 
        # connect async
        self.sock.connect((self.ip, int(self.port)))
        self.sock.setblocking(0)
        
        data = {}
        data["Username"] = self.username
        data["Secret"] = self.password
        
        print("Send command")
        self.send_cmd_async("Login", data)
        self.connect = True
        print("check connect. connect[%s]" % self.connect)
                
        return
    

    def _message_handler(self):
        data = self._message_parser()
        if data == None:
            return

        # get event type
        event = data.pop('Event', None)
        data.pop("Privilege", None)        
        print("event type. event[%s]" % event)
        
        if event == "OutCampaignEntry":
            self.message_outcampaignentry(data)
            return
        elif event == "OutCampaignCreate":
            self.message_outcampaigncreate(data)
            return
        elif event == "OutCampaignUpdate":
            self.message_outcampaignupdate(data)
            return
        elif event == "OutCampaignDelete":
            self.message_outcampaigndelete(data)
            return
        elif event == "OutPlanEntry":
            self.message_outplanentry(data)
            return
        elif event == "OutPlanCreate":
            self.message_outplancreate(data)
            return
        elif event == "OutPlanUpdate":
            self.message_outplanupdate(data)
            return
        elif event == "OutPlanDelete":
            self.message_outplandelete(data)
            return
        elif event == "OutDlmaEntry":
            self.message_outdlmaentry(data)
            return
        elif event == "OutDlmaCreate":
            self.message_outdlmacreate(data)
            return
        elif event == "OutDlmaUpdate":
            self.message_outdlmaupdate(data)
            return
        elif event == "OutDlmaDelete":
            self.message_outdlmadelete(data)
            return
        elif event == "OutDestinationEntry":
            self.message_outdestinationentry(data)
            return
        elif event == "OutDestinationCreate":
            self.message_outdestinationcreate(data)
            return
        elif event == "OutDestinationUpdate":
            self.message_outdestinationupdate(data)
            return
        elif event == "OutDestinationDelete":
            self.message_outdestinationdelete(data)
            return
        elif event == "OutDlListEntry":
            self.message_outdllistentry(data)
            return
        elif event == "OutDlListCreate":
            self.message_outdllistcreate(data)
            return
        elif event == "OutDlListUpdate":
            self.message_outdllistupdate(data)
            return
        elif event == "OutDlListDelete":
            self.message_outdllistdelete(data)
            return
        elif event == "OutDialingEntry":
            self.message_outdialingentry(data)
            return
        elif event == "OutDialingCreate":
            self.message_outdialingcreate(data)
            return
        elif event == "OutDialingUpdate":
            self.message_outdialingupdate(data)
            return
        elif event == "OutDialingDelete":
            self.message_outdialingdelete(data)
            return


        else:
            print("Could not find correct message handler. event[%s]" % event)
            return
    

    def message_outcampaignentry(self, data):
        '''
        message handler : OutCampaignEntry
        '''
        print("message_outcampaignentry")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.campaign_insert(uuid, data)
        return
        
    
    def message_outcampaigncreate(self, data):
        '''
        message handler : OutCampaignCreate
        '''
        print("message_outcampaigncreate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.campaign_insert(uuid, data)
        return
    
    
    def message_outcampaignupdate(self, data):
        '''
        message handler : OutCampaignCreate
        '''
        print("message_outcampaignupdate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.campaign_update(uuid, data)
        return


    def message_outcampaigndelete(self, data):
        '''
        message handler : OutCampaignDelete
        '''
        print("message_outcampaigndelete")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.campaign_delete(uuid)
        return


    def message_outplanentry(self, data):
        '''
        message handler : OutPlanEntry
        '''
        print("message_outplanentry")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.plan_insert(uuid, data)
        return                

    
    def message_outplancreate(self, data):
        '''
        message handler : OutPlanCreate
        '''
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.plan_insert(uuid, data)
        return
        
        
    def message_outplanupdate(self, data):
        '''
        message handler : OutPlanUpdate
        '''
        print("message_outplanupdate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.plan_update(uuid, data)
        return
        
    
    def message_outplandelete(self, data):
        '''
        message handler : OutPlanDelete
        '''
        print("message_outplandelete")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.plan_delete(uuid)
        return


    def message_outdlmaentry(self, data):
        '''
        message handler : OutDlmaEntry
        '''
        print("message_outdlmaentry")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.dlma_insert(uuid, data)
        return
        
    
    def message_outdlmacreate(self, data):
        '''
        message handler : OutDlmaCreate
        '''
        print("message_outdlmacreate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.dlma_insert(uuid, data)
        return
    
    
    def message_outdlmaupdate(self, data):
        '''
        message handler : OutDlmaCreate
        '''
        print("message_outdlmaupdate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.dlma_update(uuid, data)
        return


    def message_outdlmadelete(self, data):
        '''
        message handler : OutDlmaDelete
        '''
        print("message_outdlmadelete")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.dlma_delete(uuid)
        return


    def message_outdestinationentry(self, data):
        '''
        message handler : OutDestinationEntry
        '''
        print("message_outdestinationentry")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.destination_insert(uuid, data)
        return
        
    
    def message_outdestinationcreate(self, data):
        '''
        message handler : OutDestinationCreate
        '''
        print("message_outdestinationcreate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.destination_insert(uuid, data)
        return
    
    
    def message_outdestinationupdate(self, data):
        '''
        message handler : OutDestinationCreate
        '''
        print("message_outdestinationupdate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.destination_update(uuid, data)
        return


    def message_outdestinationdelete(self, data):
        '''
        message handler : OutDestinationDelete
        '''
        print("message_outdestinationdelete")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
        
        self.data_handler.destination_delete(uuid)
        return
    
    
    def message_outdllistentry(self, data):
        '''
        message handler : OutDlListEntry
        '''
        print("message_outdllistentry")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.diallist_insert(uuid, data)
        return


    def message_outdllistcreate(self, data):
        '''
        message handler : OutDlListCreate
        '''
        print("message_outdllistcreate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.diallist_insert(uuid, data)
        return


    def message_outdllistupdate(self, data):
        '''
        message handler : OutDlListUpdate
        '''
        print("message_outdllistupdate")
        print("Detail info. data[%s]" % data)
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.diallist_update(uuid, data)
        return


    def message_outdllistdelete(self, data):
        '''
        message handler : OutDlListDelete
        '''
        print("message_outdllistdelete")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.diallist_delete(uuid)
        return


    def message_outdialingtentry(self, data):
        '''
        message handler : OutDialingEntry
        '''
        print("message_outdialingtentry")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.dialing_insert(uuid, data)
        return


    def message_outdialingcreate(self, data):
        '''
        message handler : OutDialingCreate
        '''
        print("message_outdialingcreate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.dialing_insert(uuid, data)
        return


    def message_outdialingupdate(self, data):
        '''
        message handler : OutDialingUpdate
        '''
        print("message_outdialingupdate")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.dialing_update(uuid, data)
        return


    def message_outdialingdelete(self, data):
        '''
        message handler : OutDialingDelete
        '''
        print("message_outdialingdelete")
        if data == None or "Uuid" not in data:
            return
        
        # get uuid
        uuid = data["Uuid"]
        if uuid == None:
            return
            
        self.data_handler.dialing_delete(uuid)
        return

