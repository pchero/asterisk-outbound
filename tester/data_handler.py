# -*- coding: utf-8 -*-
"""
Created on Wed Jan  4 23:48:26 2017

@author: pchero
"""

import sqlite3 as db
import ast

class DataHandler(object):
    
    # database
    con = None     # connection
    cur = None     # cursor
    

    view_handler = None    
    
    def __init__(self):
        self.con = db.connect(":memory:")
        self.cur = self.con.cursor()
        self._create_tables()
        
        return
        
    def _create_tables(self):
        self.cur.execute("create table campaigns(uuid text not null primary key, data text)")
        self.cur.execute("create table plans(uuid text not null primary key, data text)")
        self.cur.execute("create table dlmas(uuid text not null primary key, data text)")
        self.cur.execute("create table destinations(uuid text not null primary key, data text)")
        self.cur.execute("create table diallists(uuid text not null primary key, dlma_uuid text, data text)")
        self.cur.execute("create table dialings(uuid text not null primary key, data text)")
        
        
    def _update_list_items(self, table):
        try:
            self.view_handler.update_list_items(table)
        except:
            pass
        return
        

    def set_view_handler(self, handler):
        self.view_handler = handler
    
    
    def plan_insert(self, uuid, data):
        print("plan_insert")
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """insert or ignore into plans(uuid, data) values ("%s", "%s");""" % (uuid, str(data))
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("plan")
        
        return True
    
    
    def plan_update(self, uuid, data):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False
        
        sql = """update plans set data="%s" where uuid="%s";""" % (str(data), uuid)
        self.cur.execute(sql)
        
        self._update_list_items("plan")
        print("Plan info updated.")
        return True


    def plan_delete(self, uuid):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False
        
        sql = """delete from plans where uuid="%s";""" % (uuid)
        self.cur.execute(sql)
        print("Plan info deleted. uuid[%s]" % uuid)
        
        # view handler update
        self._update_list_items("plan")

        return True

    
    def plan_get(self, uuid):
        if uuid == None:
            return None
        
        sql = """select data from plans where uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        tmp = self.cur.fetchone()
        if tmp == None:
            return None
                
        res = ast.literal_eval(str(tmp[0]))
        
        return res
        
    
    def plan_get_list_all(self):
        sql = "select uuid from plans"
        self.cur.execute(sql)
        
        res = []
        for tmp in self.cur:
            res.append(tmp[0])
                
        return res


    def campaign_insert(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """insert or ignore into campaigns(uuid, data) values ("%s", "%s");""" % (uuid, str(data))
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("campaign")
        print("Campaign inserted. uuid[%s]" % uuid)
        
        return True
        
        
    def campaign_update(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """update campaigns set data="%s" where uuid="%s";""" % (str(data), uuid)
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("campaign")
        
        print("Campaign info updated.")
        return True


    def campaign_delete(self, uuid):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False

        sql = """delete from campaigns where uuid="%s";""" % uuid
        self.cur.execute(sql)

        
        # view handler update
        self._update_list_items("campaign")

        print("Campaign info deleted. uuid[%s]" % uuid)
        return True


    def campaign_get(self, uuid):
        if uuid == None:
            return None
        
        sql = """select data from campaigns where uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        tmp = self.cur.fetchone()
        if tmp == None:
            return None
                
        res = ast.literal_eval(str(tmp[0]))
        
        return res

    
    def campaign_get_list_all(self):
        sql = "select uuid from campaigns"
        self.cur.execute(sql)
        
        res = []
        for tmp in self.cur:
            res.append(tmp[0])
                
        return res


    def dlma_insert(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """insert or ignore into dlmas(uuid, data) values ("%s", "%s");""" % (uuid, str(data))
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("dlma")
        print("Dlma inserted. uuid[%s]" % uuid)
        
        return True
        
        
    def dlma_update(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """update dlmas set data="%s" where uuid="%s";""" % (str(data), uuid)
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("dlma")
        
        print("Dlma info updated.")
        return True


    def dlma_delete(self, uuid):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False

        sql = """delete from dlmas where uuid="%s";""" % uuid
        self.cur.execute(sql)

        
        # view handler update
        self._update_list_items("dlma")

        print("Dlma info deleted. uuid[%s]" % uuid)
        return True


    def dlma_get(self, uuid):
        if uuid == None:
            return None
        
        sql = """select data from dlmas where uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        tmp = self.cur.fetchone()
        if tmp == None:
            return None
                
        res = ast.literal_eval(str(tmp[0]))
        
        return res

    
    def dlma_get_list_all(self):
        self.cur.execute("select uuid from dlmas")
        
        res = []
        for tmp in self.cur:
            res.append(tmp)
                
        return res
        
    
    def dlma_get_diallist_list_all(self, uuid):
        if uuid == None:
            return None
        
        sql = """select uuid from diallists where dlma_uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        res = []
        for tmp in self.cur:
            res.append(tmp[0])
                
        return res


    def destination_insert(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """insert or ignore into destinations(uuid, data) values ("%s", "%s");""" % (uuid, str(data))
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("destination")
        print("Destination inserted. uuid[%s]" % uuid)
        
        return True
        
        
    def destination_update(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """update destinations set data="%s" where uuid="%s";""" % (str(data), uuid)
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("destination")
        
        print("Destination info updated.")
        return True
        

    def destination_delete(self, uuid):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False

        sql = """delete from destinations where uuid="%s";""" % uuid
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("destintion")

        print("Destination info deleted. uuid[%s]" % uuid)
        return True


    def destination_get(self, uuid):
        if uuid == None:
            return None
        
        sql = """select data from destinations where uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        tmp = self.cur.fetchone()
        if tmp == None:
            return None
                
        res = ast.literal_eval(str(tmp[0]))
        
        return res

    
    def destination_get_list_all(self):
        self.cur.execute("select uuid from destinations")
        
        res = []
        for tmp in self.cur:
            res.append(tmp)
                
        return res


    def diallist_insert(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        # get dlma_uuid
        dlma_uuid = data["DlmaUuid"]
        
        sql = """insert or ignore into diallists(uuid, dlma_uuid, data) values ("%s", "%s", "%s");""" % (uuid, dlma_uuid, str(data))
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("diallist")
        print("Diallist inserted. uuid[%s]" % uuid)
        
        return True
                
        
    def diallist_update(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False
        
        # get dlma_uuid
        dlma_uuid = data["DlmaUuid"]
        
        sql = """update diallists set dlma_uuid="%s", data="%s" where uuid="%s";""" % (dlma_uuid, str(data), uuid)
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("diallist")
        print("Diallist updated. uuid[%s]" % uuid)
        
        return True


    def diallist_delete(self, uuid):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False

        sql = """delete from diallists where uuid="%s";""" % (uuid)
        self.cur.execute(sql)

        print("Diallist info deleted.")
        return


    def diallist_get(self, uuid):
        if uuid == None:
            return None
        
        sql = """select data from diallists where uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        tmp = self.cur.fetchone()
        if tmp == None:
            return None
                
        res = ast.literal_eval(str(tmp[0]))
        
        return res

    
    def diallist_get_list_all(self):
        self.cur.execute("select uuid from diallists")
        
        res = []
        for tmp in self.cur:
            res.append(tmp)
                
        return res


    def dialing_insert(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """insert or ignore into dialings(uuid, data) values ("%s", "%s");""" % (uuid, str(data))
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("dialing")
        print("Dialing inserted. uuid[%s]" % uuid)
        
        return True
        
        
    def dialing_update(self, uuid, data):
        if uuid == None or data == None:
            print("Wrong input parameter")
            return False

        sql = """update dialings set data="%s" where uuid="%s";""" % (str(data), uuid)
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("dialing")
        
        print("Dialing info updated.")
        return True
        

    def dialing_delete(self, uuid):
        # get uuid
        if uuid == None:
            print("Wrong input parameter.")
            return False

        sql = """delete from dialings where uuid="%s";""" % uuid
        self.cur.execute(sql)

        # view handler update
        self._update_list_items("dialing")

        print("Dialing info deleted. uuid[%s]" % uuid)
        return True


    def dialing_get(self, uuid):
        if uuid == None:
            return None
        
        sql = """select data from dialings where uuid="%s";"""% (uuid)
        self.cur.execute(sql)
        
        tmp = self.cur.fetchone()
        if tmp == None:
            return None
                
        res = ast.literal_eval(str(tmp[0]))
        
        return res

    
    def dialing_get_list_all(self):
        self.cur.execute("select uuid from dialings")
        
        res = []
        for tmp in self.cur:
            res.append(tmp)
                
        return res



