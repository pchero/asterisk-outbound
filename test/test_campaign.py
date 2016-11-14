# test_campaign.py
#  Created on: Nov 27, 2015
#      Author: pchero

import common

import os
import sys
import uuid

def create_campaign():
    # normal
    ast = common.acli()
    ast.conn()
    ast.sendCmd("OutCampaignCreate")
    
    return True


def main():
    ret = create_campaign()
    if ret != True:
        raise
    
    
    ast = common.Ami()
    ast.username = sys.argv[1]
    ast.password = sys.argv[2]
    if ast.conn() == False:
        print("Could not connect.")
        return 1
    
    # create campaign
    print "CampaignCreate"
    camp_name = uuid.uuid4().__str__()
    ret = ast.sendCmd("OutCampaignCreate", Name=camp_name, Detail="TestDetail")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"    
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] != "OutCampaignCreate":
            continue
        if ret["Name"] == camp_name:
            break
    
    if "Uuid" not in ret \
        or "Detail" not in ret \
        or "Name" not in ret \
        or "Status" not in ret \
        or "TmCreate" not in ret \
        or "TmDelete" not in ret \
        or "TmUpdate" not in ret:
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    if ret["Name"] != camp_name or ret["Detail"] != "TestDetail":
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    camp_uuid = ret["Uuid"]
    
    # get campaign
    print("CampaignCreateCheck")
    ret = ast.sendCmd("OutCampaignShow", Uuid=camp_uuid)
    flg = False
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == camp_uuid:
            flg = True
            break
    if flg == False:        
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    
    # update campaign
    print("CampaignUpdate")
    ret = ast.sendCmd("OutCampaignUpdate", Uuid=camp_uuid, Detail="Change")
    if ret[0]["Response"] != "Success":
        print("Could not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    flg = False
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] != "OutCampaignUpdate":
            continue
        if ret["Name"] == camp_name:
            flg = True
            break
    if flg == False:
        print("Could not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    if ret["Uuid"] != camp_uuid or ret["Detail"] != "Change":
        print("Could not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    
    # delete campaign
    ret = ast.sendCmd("OutCampaignDelete", Uuid=camp_uuid)
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    flg = False
    for i in range(100):
        ret = ast.recvEvt()
        if ret["Event"] != "OutCampaignDelete":
            continue
        if ret["Uuid"] == camp_uuid:
            flg = True
            break
    if flg == False:
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"
    
    # get campaign
    ret = ast.sendCmd("OutCampaignShow", Uuid=camp_uuid)
    flg = True
    for i in range(len(ret)):
        msg = ret[i]
        if "Uuid" not in msg:
            continue
        if msg["Uuid"] == camp_uuid:
            flg = False
            break
    if flg == False:        
        print("Couldn not pass the test_campaign. ret[%s]" % ret)
        raise "test_campaign"

    
    return 0

if __name__ == '__main__':
    main()
