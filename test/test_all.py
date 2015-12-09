# test_all.py
#  Created on: Dec 8, 2015
#      Author: pchero

import os
import sys

def main():
    try:
        ret = os.system("python ./test_campaign.py %s %s" % (sys.argv[1], sys.argv[2]))
        if ret != 0:
            print("Could not pass the test.")
            return
        
        ret = os.system("python ./test_plan.py %s %s" % (sys.argv[1], sys.argv[2]))
        if ret != 0:
            print("Could not pass the test.")
            return
        
        ret = os.system("python ./test_dlma.py %s %s" % (sys.argv[1], sys.argv[2]))
        if ret != 0:
            print("Could not pass the test.")
            return
        
        ret = os.system("python ./test_dialing.py %s %s" % (sys.argv[1], sys.argv[2]))
        if ret != 0:
            print("Could not pass the test.")
            return

    except Exception as e:
        print e

if __name__ == '__main__':
    main()

