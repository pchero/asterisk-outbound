# test_campaign.py
#  Created on: Nov 27, 2015
#      Author: pchero

import common

import os
import sys
import uuid


def get_dlma_by_name(name):
    '''
    Get dlma info by name
    '''
    ast = common.acli()
    ast.conn()
    
    ast.sendCmd("OutDlmaShow")
    res = ast.recvArr()
        
    size = len(res)
    for i in range(size):
        res_dict = common.make_dict(res[i])
        
        if name == res_dict["Name"]:
            return res[i]
            
    return None


def get_dlma(uuid_str):
    '''
    Get dlma info corresponding uuid.
    '''
    
    if uuid_str == None:
        return None
    
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutDlmaShow", Uuid=uuid_str)
    res_dict = common.make_dict(res)
    if "Response" not in res_dict or res_dict["Response"] == "Error":
        return None
    
    res = ast.recvArr()

    return res[0]


def delete_dlma(uuid_str):
    '''
    Delete dlma info corresponding uuid.
    '''
    
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutDlmaDelete", Uuid=uuid_str)
    res_dict = common.make_dict(res)
    
    if res_dict["Response"] != "Success":
        return False
    
    return True


def verify_dlma(obj, **args):
    '''
    verify obj items.
    if args is given, check the given item
    '''
    items = {}
    items["Uuid"] = None 
    items["Name"] = None
    items["Detail"] = None
    items["DlTable"] = None
    items["Variable"] = None
    items["TmCreate"] = None
    items["TmDelete"] = None
    items["TmUpdate"] = None

    # update given item
    for key, value in args.items():
        if key not in items:
            print("The key is not in item. key[%s]" % key)
            return False
        items[key] = value
    
    ret = common.verify_items(obj, items)
    return ret


def test_dlma_create_no_arg():
    '''
    test dlma create with no options
    '''
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutDlmaCreate")
    res_dict = common.make_dict(res)    
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_dlma_create_no_arg")

    print("Finished test_dlma_create_no_arg")
    return


def test_dlma_create_name():
    '''
    test dlma create with name option
    '''
    ast = common.acli()
    ast.conn()
    
    # create info with random name
    name = uuid.uuid4().__str__()
    res = ast.sendCmd("OutDlmaCreate", Name=name)
    res_dict = common.make_dict(res)
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_dlma_create_name")
    
    # get info by name
    res = get_dlma_by_name(name)
    if res == None:
        raise Exception("Failed test_dlma_create_name")
    res_dict = common.make_dict(res)
    uuid_str = res_dict["Uuid"]
    
    # verify the dlma info
    ret = verify_dlma(res, Name=name)
    if ret != True:
        raise Exception("Failed test_dlma_create_name")    
        
    # delete
    delete_dlma(uuid_str)
        
    # get info by uuid
    res = get_dlma(uuid_str)
    if res != None:
        raise Exception("Failed test_dlma_create_name.")

    print("Finished test_dlma_create_name")
    return;


def test_dlma_update_name():
    '''
    test dlma create with name option
    '''
    ast = common.acli()
    ast.conn()
    
    # create dlma with random name
    name = uuid.uuid4().__str__()
    res = ast.sendCmd("OutDlmaCreate", Name=name)
    res_dict = common.make_dict(res)
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_dlma_update_name")
    
    # get dlma info by name
    res = get_dlma_by_name(name)
    if res == None:
        raise Exception("Failed test_dlma_update_name")
    res_dict = common.make_dict(res)
    uuid_org = res_dict["Uuid"]
    
    # verify the dlma info
    ret = verify_dlma(res, Name=name)
    if ret != True:
        raise Exception("Failed test_dlma_update_name")    
    
    # update dlma
    name_new = uuid.uuid4().__str__()
    res = ast.sendCmd("OutDlmaUpdate", Uuid=uuid_org, Name=name_new)
    res_update = common.make_dict(res)
    if res_update["Response"] != "Success":
        raise Exception("Failed test_dlma_update_name")
    
    # get plan info
    res = get_dlma(uuid_org)
    ret = verify_dlma(res, Uuid=uuid_org, Name=name_new)
    if ret != True:
        raise Exception("Failed test_dlma_update_name")
    
    # delete dlma
    delete_dlma(uuid_org)
    
    # get dlma info by uuid
    res = get_dlma(uuid_org)
    if res != None:
        raise Exception("Failed test_dlma_update_name.")

    print("Finished test_dlma_update_name")
    return

def main():
    test_dlma_create_no_arg()
    test_dlma_create_name()
    test_dlma_update_name()
    
    print("Finished dlma test")
    return

if __name__ == '__main__':
    main()


