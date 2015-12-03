# test_dlma.py
#  Created on: Dec 2, 2015
#      Author: pchero

 
import os
import sys

import test_common    
            
def main():
    ast = test_common.Ami()
    ast.username = sys.argv[1]
    ast.password = sys.argv[2]
    if ast.conn() == False:
        print("Could not connect.")
        return 1
    
    # create dlma
    ret = ast.sendCmd("OutDlmaCreate", Name="TestDlma", Detail="TestDetail")
    res = ret
    if res[0]["Response"] != "Success":
        print("Couldn not pass the test_dlma. res[%s]" % res)
        raise "test_dlma"
    for i in range(10):
        evt = ast.recvEvt()
        if evt["Event"] == "OutDlmaCreate":
            break
    if evt["Name"] != "TestDlma" or evt["Detail"] != "TestDetail":
        print("Couldn not pass the test_dlma. ret[%s]" % evt)
        raise "test_dlma"
    test_uuid = evt["Uuid"]
    
    # get dlma
    ret = ast.sendCmd("OutDlmaShow", Uuid=test_uuid)
    flg = False
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == test_uuid:
            flg = True
            break
    if flg == False:        
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    
    # delete dlma
    ret = ast.sendCmd("OutDlmaDelete", Uuid=test_uuid)
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    for i in range(10):
        ret = ast.recvEvt()
        if ret["Event"] == "OutDlmaDelete":
            break
    if ret["Uuid"] != test_uuid:
        print("Could not pass the test_dlma. ret[%s]" % ret)
        raise "test_dlma"
    
    # get campaign
    ret = ast.sendCmd("OutDlmaShow", Uuid=test_uuid)
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == test_uuid:
            print("Could not pass the test_dlma. ret[%s], uuid[%s]" % (ret, test_uuid))
            raise "test_dlma"
    
    return 0


if __name__ == '__main__':
    main()
