# test_campaign.py
#  Created on: Nov 27, 2015
#      Author: pchero

import common

import os
import sys
import uuid


def get_destination_by_name(name):
    '''
    Get destination info by name
    '''
    ast = common.acli()
    ast.conn()
    
    ast.sendCmd("OutDestinationShow")
    res = ast.recvArr()
        
    size = len(res)
    for i in range(size):
        res_dict = common.make_dict(res[i])
        
        if name == res_dict["Name"]:
            return res[i]
            
    return None
    

def get_destination(uuid_str):
    '''
    Get destination info corresponding uuid.
    '''
    
    if uuid_str == None:
        return None
    
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutDestinationShow", Uuid=uuid_str)
    res_dict = common.make_dict(res)
    if "Response" not in res_dict or res_dict["Response"] == "Error":
        return None
    
    res = ast.recvArr()

    return res[0]


def delete_destination(uuid_str):
    '''
    Delete destination info corresponding uuid.
    '''
    
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutDestinationDelete", Uuid=uuid_str)
    res_dict = common.make_dict(res)
    
    if res_dict["Response"] != "Success":
        return False
    
    return True

def verify_destination(obj, **args):
    '''
    verify obj items.
    if args is given, check the given item
    '''
    items = {}
    items["Uuid"] = None 
    items["Name"] = None
    items["Detail"] = None
    items["Type"] = None
    items["Exten"] = None
    items["Context"] = None
    items["Priority"] = None
    items["Application"] = None
    items["Data"] = None
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


def test_destination_create_no_arg():
    '''
    test destination create with no options
    '''
    ast = common.acli()
    ast.conn()
    
    res = ast.sendCmd("OutDestinationCreate")
    res_dict = common.make_dict(res)    
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_destination_create_no_arg")

    print("Finished test_destination_create_no_arg")
    return


def test_destination_create_name():
    '''
    test destination create with name option
    '''
    ast = common.acli()
    ast.conn()
    
    # create info with random name
    name = uuid.uuid4().__str__()
    res = ast.sendCmd("OutDestinationCreate", Name=name)
    res_dict = common.make_dict(res)
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_destination_create_name")
    
    # get .info by name
    res = get_destination_by_name(name)
    if res == None:
        raise Exception("Failed test_destination_create_name")
    res_dict = common.make_dict(res)
    uuid_str = res_dict["Uuid"]
    
    # verify the plan info
    ret = verify_destination(res, Name=name)
    if ret != True:
        raise Exception("Failed test_destination_create_name")    
        
    # delete
    delete_destination(uuid_str)
        
    # get info by uuid
    res = get_destination(uuid_str)
    if res != None:
        raise Exception("Failed test_destination_create_name.")

    print("Finished test_destination_create_name")
    return;


def test_destination_update_name():
    '''
    test destination create with name option
    '''
    ast = common.acli()
    ast.conn()
    
    # create plan with random name
    name = uuid.uuid4().__str__()
    res = ast.sendCmd("OutDestinationCreate", Name=name)
    res_dict = common.make_dict(res)
    if res_dict["Response"] != "Success":
        raise Exception("Failed test_destination_update_name")
    
    # get plan info by name
    res = get_destination_by_name(name)
    if res == None:
        raise Exception("Failed test_destination_update_name")
    res_dict = common.make_dict(res)
    uuid_org = res_dict["Uuid"]
    
    # verify the destination info
    ret = verify_destination(res, Name=name)
    if ret != True:
        raise Exception("Failed test_destination_update_name")    
    
    # update destination
    name_new = uuid.uuid4().__str__()
    res = ast.sendCmd("OutDestinationUpdate", Uuid=uuid_org, Name=name_new)
    res_update = common.make_dict(res)
    if res_update["Response"] != "Success":
        raise Exception("Failed test_destination_update_name")
    
    # get destination info
    res = get_destination(uuid_org)
    ret = verify_destination(res, Uuid=uuid_org, Name=name_new)
    if ret != True:
        raise Exception("Failed test_destination_update_name")
    
    # delete destination
    delete_destination(uuid_org)
    
    # get plan info by uuid
    res = get_destination(uuid_org)
    if res != None:
        raise Exception("Failed test_destination_update_name.")

    print("Finished test_destination_update_name")
    return

def main():
    test_destination_create_no_arg()
    test_destination_create_name()
    test_destination_update_name()
    
    print("Finished destination test")
    return

if __name__ == '__main__':
    main()
