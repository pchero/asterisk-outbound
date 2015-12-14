# test_campaign.py
#  Created on: Nov 27, 2015
#      Author: pchero

import common

import os
import sys
import uuid

def main():
    ast = common.Ami()
    ast.username = sys.argv[1]
    ast.password = sys.argv[2]
    if ast.conn() == False:
        print("Could not connect.")
        return 1
    
    # create plan
    print("Create plan")
    plan_name = uuid.uuid4().__str__()
    ret = ast.sendCmd("OutPlanCreate", Name=plan_name, Detail="TestDetail")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] != "OutPlanCreate":
            continue
        if ret["Name"] != plan_name:
            continue
        break
        
    # item check
    if "Uuid" not in ret \
        or "Name" not in ret \
        or "Detail" not in ret \
        or "DialMode" not in ret \
        or "DialTimeout" not in ret \
        or "CallerId" not in ret \
        or "AnswerHandle" not in ret \
        or "DlEndHandle" not in ret \
        or "RetryDelay" not in ret \
        or "TrunkName" not in ret \
        or "QueueName" not in ret \
        or "AmdMode" not in ret \
        or "MaxRetryCnt1" not in ret \
        or "MaxRetryCnt2" not in ret \
        or "MaxRetryCnt3" not in ret \
        or "MaxRetryCnt4" not in ret \
        or "MaxRetryCnt5" not in ret \
        or "MaxRetryCnt6" not in ret \
        or "MaxRetryCnt7" not in ret \
        or "MaxRetryCnt8" not in ret \
        or "TmCreate" not in ret \
        or "TmDelete" not in ret \
        or "TmUpdate" not in ret:
        
            print("Couldn not pass the test_plan. ret[%s]" % ret)
            raise "test_plan"
    if ret["Name"] != plan_name or ret["Detail"] != "TestDetail":
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    plan_uuid = ret["Uuid"]
    
    # get plan
    print("Get plan")
    ret = ast.sendCmd("OutPlanShow", Uuid=plan_uuid)
    flg = False
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == plan_uuid:
            flg = True
            break
    if flg == False:        
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    
    # update plan
    print("Update plan")
    ret = ast.sendCmd("OutPlanUpdate", Uuid=plan_uuid, Detail="Change")
    if ret[0]["Response"] != "Success":
        print("Could not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] == "OutPlanUpdate":
            break
    if ret["Uuid"] != plan_uuid or ret["Detail"] != "Change":
        print("Could not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    
    # delete plan
    print("Delete plan")
    ret = ast.sendCmd("OutPlanDelete", Uuid=plan_uuid)
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] == "OutPlanDelete":
            break
    if ret["Uuid"] != plan_uuid:
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    
    # get plan
    print("Get plan")
    ret = ast.sendCmd("OutPlanShow", Uuid=plan_uuid)
    flg = True
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == plan_uuid:
            flg = False
            break
    if flg == False:        
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"
    
    print("Get plan all")
    ret = ast.sendCmd("OutPlanShow")
#     print ret
    flg = True
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == plan_uuid:
            flg = False
            break
    if flg == False:        
        print("Couldn not pass the test_plan. ret[%s]" % ret)
        raise "test_plan"

    return 0

if __name__ == '__main__':
    main()
