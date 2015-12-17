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
    
    # Create tmp dlma
    print("Create dlma for dl_list")
    dlma_name = uuid.uuid4().__str__()
    ret = ast.sendCmd("OutDlmaCreate", Name=dlma_name, Detail="TestDetail")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] != "OutDlmaCreate":
            continue
        if ret["Name"] != dlma_name:
            continue
        break
    dlma_uuid = ret["Uuid"]
    
    # create dl_list
    print("Create dl_list")
    dl_name = uuid.uuid4().__str__()
    ret = ast.sendCmd("OutDlListCreate", DlmaUuid=dlma_uuid, Name=dl_name, Detail="TestDetail", Number1="111-111-1111")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"
    
    # get dl_list
    print("Get dl_list")
    ret = ast.sendCmd("OutDlListShow", DlmaUuid=dlma_uuid, Count=100)
    for j_dl_org in ret:
        if "Event" not in j_dl_org or j_dl_org["Event"] != "OutDlListEntry":
            continue
        break
    
    # update dl_list
    print("Update dl_list")
    ret = ast.sendCmd("OutDlListUpdate", Uuid=j_dl_org["Uuid"], Detail="Update detail", Number1="111-111-1111")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"
        
    # get dl_list
    ret = ast.sendCmd("OutDlListShow", Uuid=j_dl_org["Uuid"])
    j_dl = ret[1]
    if j_dl["Uuid"] != j_dl_org["Uuid"]:
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"
    
    if j_dl["Uuid"] != j_dl_org["Uuid"] or j_dl["Detail"] != "Update detail":
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"

    
    # delete dl_list
    print("Delete dl_list")
    ret = ast.sendCmd("OutDlListDelete", Uuid=j_dl_org["Uuid"])
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"

    # get dl_list
    print("Get dl_list")
    ret = ast.sendCmd("OutDlListShow", Uuid=j_dl_org["Uuid"])
    for j_dl in ret:
        if "Event" in j_dl and j_dl["Event"] == "OutDlListEntry":
            print("Couldn not pass the test_dl_list. ret[%s]" % ret)
            raise "test_dl_list"
    
    # delete dlma
    print("Delete dlma for dl_list")
    ret = ast.sendCmd("OutDlmaDelete", Uuid=dlma_uuid)
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dl_list. ret[%s]" % ret)
        raise "test_dl_list"
    
    return
    
#     
#     dl_name = uuid.uuid4().__str__()
#     ret = ast.sendCmd("OutDlListCreate", DlmaUuid=dlma_uuid, Name=dl_name, Detail="TestDetail", Number1="111-111-1111")
#     if ret[0]["Response"] != "Success":
#         print("Couldn not pass the test_dl_list. ret[%s]" % ret)
#         raise "test_dl_list"
#     
#     for i in range(100):
#         ret = ast.recvEvt()
#         if ret["Event"] != "OutPlanCreate":
#             continue
#         if ret["Name"] != dl_name:
#             continue
#         break
#         
#     # item check
#     if "Uuid" not in ret \
#         or "Name" not in ret \
#         or "Detail" not in ret \
#         or "DialMode" not in ret \
#         or "DialTimeout" not in ret \
#         or "CallerId" not in ret \
#         or "AnswerHandle" not in ret \
#         or "DlEndHandle" not in ret \
#         or "RetryDelay" not in ret \
#         or "TrunkName" not in ret \
#         or "QueueName" not in ret \
#         or "AmdMode" not in ret \
#         or "MaxRetryCnt1" not in ret \
#         or "MaxRetryCnt2" not in ret \
#         or "MaxRetryCnt3" not in ret \
#         or "MaxRetryCnt4" not in ret \
#         or "MaxRetryCnt5" not in ret \
#         or "MaxRetryCnt6" not in ret \
#         or "MaxRetryCnt7" not in ret \
#         or "MaxRetryCnt8" not in ret \
#         or "TmCreate" not in ret \
#         or "TmDelete" not in ret \
#         or "TmUpdate" not in ret:
#         
#             print("Couldn not pass the test_plan. ret[%s]" % ret)
#             raise "test_plan"
#     if ret["Name"] != dl_name or ret["Detail"] != "TestDetail":
#         print("Couldn not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     plan_uuid = ret["Uuid"]
#     
#     # get plan
#     print("Get plan")
#     ret = ast.sendCmd("OutPlanShow", Uuid=plan_uuid)
#     flg = False
#     for i in range(len(ret)):
#         msg = ret[i]
#         if "Uuid" not in msg:
#             continue
#         if msg["Uuid"] == plan_uuid:
#             flg = True
#             break
#     if flg == False:        
#         print("Couldn not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     
#     # update plan
#     print("Update plan")
#     ret = ast.sendCmd("OutPlanUpdate", Uuid=plan_uuid, Detail="Change")
#     if ret[0]["Response"] != "Success":
#         print("Could not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     for i in range(100):
#         ret = ast.recvEvt()
#         if ret["Event"] == "OutPlanUpdate":
#             break
#     if ret["Uuid"] != plan_uuid or ret["Detail"] != "Change":
#         print("Could not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     
#     # delete plan
#     print("Delete plan")
#     ret = ast.sendCmd("OutPlanDelete", Uuid=plan_uuid)
#     if ret[0]["Response"] != "Success":
#         print("Couldn not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     for i in range(100):
#         ret = ast.recvEvt()
#         if ret["Event"] == "OutPlanDelete":
#             break
#     if ret["Uuid"] != plan_uuid:
#         print("Couldn not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     
#     # get plan
#     print("Get plan")
#     ret = ast.sendCmd("OutPlanShow", Uuid=plan_uuid)
#     flg = True
#     for i in range(len(ret)):
#         msg = ret[i]
#         if "Uuid" not in msg:
#             continue
#         if msg["Uuid"] == plan_uuid:
#             flg = False
#             break
#     if flg == False:        
#         print("Couldn not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"
#     
#     print("Get plan all")
#     ret = ast.sendCmd("OutPlanShow")
# #     print ret
#     flg = True
#     for i in range(len(ret)):
#         msg = ret[i]
#         if "Uuid" not in msg:
#             continue
#         if msg["Uuid"] == plan_uuid:
#             flg = False
#             break
#     if flg == False:        
#         print("Couldn not pass the test_plan. ret[%s]" % ret)
#         raise "test_plan"

    return 0

if __name__ == '__main__':
    main()
