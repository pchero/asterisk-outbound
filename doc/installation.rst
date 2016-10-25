.. installation

************
Installation
************

Requirements
============

::

    Asterisk-13.6 or later.
    sqlite3
    libevent2

Install
=======

::

   $ make
   $ sudo make install
   $ sudo cp conf/res_outbound.conf /etc/asterisk 

Configuration
=============

::

   cat /etc/asterisk/res_outbound.conf


   ;
   ; Configuration file for res_outbound
   ;---------------------------------
   ;
   ; The res_outbound can managing outbound calls.
   ;
   ; See https://github.com/pchero/asterisk-outbound
   
   [general]
   
   ; result type 1:json
   result_type = 1
   result_filename = /var/lib/asterisk/astout.result
   
   ; fast event time delay(us). Default 100000. (0.1 sec)
   event_time_fast = 100000
   
   ; slow event time delay(us). Default 3000000. (3 sec) 
   event_time_slow = 3000000
   
   ; Save ami events.(DEBUGING ONLY)
   ami_event_debug = 0
  
    
   [database]
   
   ; database type. 1:sqlite3
   db_type = 1
   
   ; database meta data
   db_sqlite3_data = /var/lib/asterisk/astout.sqlite3


