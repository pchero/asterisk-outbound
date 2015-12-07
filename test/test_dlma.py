# test_dlma.py
#  Created on: Dec 2, 2015
#      Author: pchero

 
import os
import sys

import test_common    


import test_common

import os
import sys

def main():
    ast = test_common.Ami()
    ast.username = sys.argv[1]
    ast.password = sys.argv[2]
    if ast.conn() == False:
        print("Could not connect.")
        return 1
    
    # create dlma
    print("Create dlma")
    ret = ast.sendCmd("OutDlmaCreate", Name="TestDlma", Detail="TestDetail")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    for i in range(10):
        ret = ast.recvEvt()
        if ret["Event"] == "OutDlmaCreate":
            break
        
    # item check
    if "Uuid" not in ret \
        or "Name" not in ret \
        or "Detail" not in ret \
        or "DlTable" not in ret \
        or "TmCreate" not in ret \
        or "TmDelete" not in ret \
        or "TmUpdate" not in ret:
        
            print("Couldn not pass the test_plan. ret[%s]" % ret)
            raise "test_dlma"
    if ret["Name"] != "TestDlma" or ret["Detail"] != "TestDetail":
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    dlma_uuid = ret["Uuid"]
    
    # get dlma
    print("Get dlma")
    ret = ast.sendCmd("OutDlmaShow", Uuid=dlma_uuid)
    flg = False
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == dlma_uuid:
            flg = True
            break
    if flg == False:        
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    
    # update dlma
    print("Update dlma")
    ret = ast.sendCmd("OutDlmaUpdate", Uuid=dlma_uuid, Detail="Change")
    if ret[0]["Response"] != "Success":
        print("Could not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    for i in range(10):
        ret = ast.recvEvt()
        if ret["Event"] == "OutDlmaUpdate":
            break
    if ret["Uuid"] != dlma_uuid or ret["Detail"] != "Change":
        print("Could not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    
    # delete dlma
    print("Delete dlma")
    ret = ast.sendCmd("OutDlmaDelete", Uuid=dlma_uuid)
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    for i in range(10):
        ret = ast.recvEvt()
        if ret["Event"] == "OutDlmaDelete":
            break
    if ret["Uuid"] != dlma_uuid:
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    
    # get dlma
    print("Get dlma")
    ret = ast.sendCmd("OutDlmaShow", Uuid=dlma_uuid)
    flg = True
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == dlma_uuid:
            flg = False
            break
    if flg == False:        
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"

    return 0

if __name__ == '__main__':
    main()

