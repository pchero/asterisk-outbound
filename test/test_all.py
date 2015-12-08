# test_all.py
#  Created on: Dec 8, 2015
#      Author: pchero

import os
import sys

def main():
    try:
        ret = os.system("python ./test_campaign.py %s %s" % (sys.argv[1], sys.argv[2]))
        print ret
        
        ret = os.system("python ./test_plan.py %s %s" % (sys.argv[1], sys.argv[2]))
        print ret
        
        ret = os.system("python ./test_dlma.py %s %s" % (sys.argv[1], sys.argv[2]))
        print ret
        
    except Exception as e:
        print e

if __name__ == '__main__':
    main()

