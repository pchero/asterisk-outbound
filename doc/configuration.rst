.. configuration

::

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
   result_info_enable = 0
   
   ; write history events to the result.
   ; required set history_events_enable
   result_history_events_enable = 0
   
   ; fast event time delay(us). Default 100000. (0.1 sec)
   event_time_fast = 100000
   
   ; slow event time delay(us). Default 3000000. (3 sec)
   event_time_slow = 3000000
   
   ; save ami events.
   ; makes huge amount of memory usage.
   history_events_enable = 0
   
   
   [database]
   
   ; database type. 1:sqlite3
   db_type = 1
   
   ; database meta data
   db_sqlite3_data = /var/lib/asterisk/astout.sqlite3


