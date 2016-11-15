# test_campaign.py
#  Created on: Nov 27, 2015
#      Author: pchero

import common

import os
import sys
import uuid


def get_plan_by_name(name):
    '''
    Get plan info by name
    '''
    ast = common.acli()
    ast.conn()
    
    ast.sendCmd("OutPlanShow")
    res = ast.recvArr()
        
    size = len(res)
    for i in range(size):
        res_dict = common.make_dict(res[i])
        
        if name == res_dict["Name"]:
            return res[i]
            
    return None
    

def get_plan(uuid_str):
    '''
    Get plan info corresponding uuid.
    '''
    
    if uuid_str == None:
        return None
    
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutPlanShow", Uuid=uuid_str)
    res_dict = common.make_dict(res)
    if "Response" not in res_dict or res_dict["Response"] == "Error":
        return None
    
    res = ast.recvArr()

    return res[0]

def delete_plan(uuid_str):
    '''
    Delete plan info corresponding uuid.
    '''
    
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutPlanDelete", Uuid=uuid_str)
    res_dict = common.make_dict(res)
    
    if res_dict["Response"] != "Success":
        raise Exception("Failed plan_delete_simple")
    
    return True

def verify_plan(plan, **args):
    '''
    verify plan items.
    if args is given, check the given item
    '''
    items = {}
    items["Uuid"] = None 
    items["Name"] = None
    items["Detail"] = None
    items["DialMode"] = None
    items["DialTimeout"] = None
    items["CallerId"] = None
    items["DlEndHandle"] = None
    items["RetryDelay"] = None
    items["TrunkName"] = None
    items["TechName"] = None
    items["Variable"] = None
    items["MaxRetryCnt1"] = None
    items["MaxRetryCnt2"] = None
    items["MaxRetryCnt3"] = None
    items["MaxRetryCnt4"] = None
    items["MaxRetryCnt5"] = None
    items["MaxRetryCnt6"] = None
    items["MaxRetryCnt7"] = None
    items["MaxRetryCnt8"] = None
    items["TmCreate"] = None
    items["TmDelete"] = None
    items["TmUpdate"] = None

    # update given item
    for key, value in args.items():
        if key not in items:
            print("The key is not in item. key[%s]" % key)
            return False
        items[key] = value
    
    ret = common.verify_items(plan, items)
    return ret

def test_plan_create_no_arg():
    '''
    test plan create with no options
    '''
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutPlanCreate")
    res_dict = common.make_dict(res)    
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_plan_create_no_arg")

    print("Finished test_plan_create_no_arg")
    return

def test_plan_create_name():
    '''
    test plan create with name option
    '''
    ast = common.acli()
    ast.conn()
    
    # create plan with random name
    name = uuid.uuid4().__str__()
    res = ast.sendCmd("OutPlanCreate", Name=name)
    res_dict = common.make_dict(res)
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_plan_create_name")
    
    # get plan info by name
    res = get_plan_by_name(name)
    if res == None:
        raise Exception("Failed test_plan_create_name")
    res_dict = common.make_dict(res)
    uuid_str = res_dict["Uuid"]
    
    # verify the plan info
    ret = verify_plan(res, Name=name)
    if ret != True:
        raise Exception("Failed test_plan_create_name")    
        
    # delete plan
    delete_plan(uuid_str)
        
    # get plan info by uuid
    res = get_plan(uuid_str)
    if res != None:
        raise Exception("Failed plan_create.")

    print("Finished test_plan_create_name")
    return;


def test_plan_update_name():
    '''
    test plan create with name option
    '''
    ast = common.acli()
    ast.conn()
    
    # create plan with random name
    name = uuid.uuid4().__str__()
    res = ast.sendCmd("OutPlanCreate", Name=name)
    res_dict = common.make_dict(res)
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_plan_update_name")
    
    # get plan info by name
    res = get_plan_by_name(name)
    if res == None:
        raise Exception("Failed test_plan_update_name")
    res_dict = common.make_dict(res)
    uuid_org = res_dict["Uuid"]
    
    # verify the plan info
    ret = verify_plan(res, Name=name)
    if ret != True:
        raise Exception("Failed test_plan_update_name")    
    
    # update plan
    name_new = uuid.uuid4().__str__()
    res = ast.sendCmd("OutPlanUpdate", Uuid=uuid_org, Name=name_new)
    res_update = common.make_dict(res)
    if res_update["Response"] != "Success":
        raise Exception("Failed test_plan_update_name")
    
    # get plan info
    res = get_plan(uuid_org)
    ret = verify_plan(res, Uuid=uuid_org, Name=name_new)
    if ret != True:
        raise Exception("Failed test_plan_update_name")
    
    # delete plan
    delete_plan(res_dict["Uuid"])
    
    # get plan info by uuid
    res = get_plan(uuid_org)
    if res != None:
        raise Exception("Failed test_plan_update_name.")

    print("Finished test_plan_update_name")
    return

def main():
    test_plan_create_no_arg()
    test_plan_create_name()
    test_plan_update_name()
    
    print("Finished plan test")
    return

if __name__ == '__main__':
    main()
