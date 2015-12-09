# test_dialing.py
#  Created on: Dec 9, 2015
#      Author: pchero

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
    
    # get dialing
    print("Getting dialing all")
    ret = ast.sendCmd("OutDialingShow")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dialing. ret[%s]" % ret)
        raise "test_dialing"
    
    # get summary
    print("Getting dialing summary")
    ret = ast.sendCmd("OutDialingSummary")
    if ret[0]["Response"] != "Success":
        print("Couldn not pass the test_dialing. ret[%s]" % ret)
        raise "test_dialing"

    return 0

if __name__ == '__main__':
    main()