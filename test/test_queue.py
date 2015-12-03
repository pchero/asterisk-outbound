# test_queue.py
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
    ret = ast.sendCmd("OutQueueCreate", Name="TestDlma", Detail="TestDetail")
    res = ret
    if res[0]["Response"] != "Success":
        print("Couldn not pass the test_queue. res[%s]" % res)
        raise "test_queue"
    for i in range(10):
        evt = ast.recvEvt()
        if evt["Event"] == "OutQueueCreate":
            break
    if evt["Name"] != "TestDlma" or evt["Detail"] != "TestDetail":
        print("Couldn not pass the test_queue. ret[%s]" % evt)
        raise "test_queue"
    test_uuid = evt["Uuid"]
    
    # get dlma
    ret = ast.sendCmd("OutQueueShow", Uuid=test_uuid)
    flg = False
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == test_uuid:
            flg = True
            break
    if flg == False:        
        print("Couldn not pass the test_queue. ret[%s]" % ret)
        raise "test_queue"
    
    # delete dlma
    ret = ast.sendCmd("OutQueueDelete", Uuid=test_uuid)
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_queue. ret[%s]" % ret)
        raise "test_queue"
    for i in range(10):
        ret = ast.recvEvt()
        if ret["Event"] == "OutQueueDelete":
            break
    if ret["Uuid"] != test_uuid:
        print("Could not pass the test_queue. ret[%s]" % ret)
        raise "test_queue"
    
    # get campaign
    ret = ast.sendCmd("OutQueueShow", Uuid=test_uuid)
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == test_uuid:
            print("Could not pass the test_queue. ret[%s], uuid[%s]" % (ret, test_uuid))
            raise "test_queue"
    
    return 0


if __name__ == '__main__':
    main()
