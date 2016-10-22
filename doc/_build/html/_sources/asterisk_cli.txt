.. asterisk_cli


************
Asterisk CLI
************

help out
========

::

   pluto*CLI> help out
   out create campaign            -- Create new campaign
   out delete campaign            -- Delete campaign
   out set status {start|starting|stop|stopping|pause|pausing} on -- Set campaign parameters
   out show campaigns             -- List all defined outbound campaigns
   out show campaign              -- Shows detail campaign info
   out show destinations          -- List all defined outbound destinations
   out show destination           -- Show detail given destination info
   out show dialings              -- List currently on serviced dialings
   out show dlma list             -- Show list of dlma dial list
   out show dlmas                 -- List all defined outbound dlmas
   out show dlma                  -- Show detail given dlma info
   out show dl                    -- Show detail given dl info
   out show dls                   -- Show list of dlma dial list
   out show plans                 -- List all defined outbound plans
   out show plan                  -- Show detail given plan info



out show plans
==============

Example
-------

::

   pluto*CLI> out show plans
   Uuid  
   4ea35c4b-c2db-4a22-baef-443b5fadd677 sales_plan           simple sales plan           1       30000            sip/      
   
out show plan <plan-uuid>
=========================

Example
-------

::

   pluto*CLI> out show plan 4ea35c4b-c2db-4a22-baef-443b5fadd677
   Plan detail info. plan-uuid[4ea35c4b-c2db-4a22-baef-443b5fadd677]
   
   {
     "uuid": "4ea35c4b-c2db-4a22-baef-443b5fadd677",
     "name": "sales_plan",
     "detail": "simple sales plan",
     "in_use": 1,
     "uui_field": null,
     "dial_mode": 1,
     "dial_timeout": 30000,
     "caller_id": null,
     "dl_end_handle": 1,
     "retry_delay": 50000,
     "trunk_name": null,
     "tech_name": "sip/",
     "service_level": 0,
     "max_retry_cnt_1": 5,
     "max_retry_cnt_2": 5,
     "max_retry_cnt_3": 5,
     "max_retry_cnt_4": 5,
     "max_retry_cnt_5": 5,
     "max_retry_cnt_6": 5,
     "max_retry_cnt_7": 5,
     "max_retry_cnt_8": 5,
     "tm_create": "2016-10-22T12:45:58.868877001Z",
     "tm_delete": null,
     "tm_update": null
   }

out show dlmas
==============

Example
-------

::
   
   pluto*CLI> out show dlmas
   Uuid                                 Name                                     Detail                                   Table                         
   acc994d2-04d9-4a53-bfcf-50c96ff924bc DialListMaster_Sales                     Test Dlma description                    acc994d2_04d9_4a53_bfcf_50c96f

out show dlma <dlma-uuid>
=========================

Example
-------

::

   pluto*CLI> out show dlma acc994d2-04d9-4a53-bfcf-50c96ff924bc
   Dlma detail info. dlma-uuid[acc994d2-04d9-4a53-bfcf-50c96ff924bc]
   
   {
     "uuid": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
     "name": "DialListMaster_Sales",
     "detail": "Test Dlma description",
     "dl_table": "acc994d2_04d9_4a53_bfcf_50c96ff924bc",
     "in_use": 1,
     "tm_create": "2016-10-22T13:29:35.820096127Z",
     "tm_delete": null,
     "tm_update": null
   }


out show dlma list <dlma-uuid> <count=100>
==========================================

::

   pluto*CLI> help out show dlma list
   Usage: out show dlma list <dlma-uuid> <count=100>
         Lists count of dial list.

::

   pluto*CLI> out show dlma list acc994d2-04d9-4a53-bfcf-50c96ff924bc
   Uuid                                 Name       Detail               Num1                 Num2                 Num3                 Num4                 Num5                 Num6                 Num7                 Num8                
   f9dfa7d2-5223-4b63-aca1-881ebacae420 client 01  Dial to client 01    300                                                                                                                                                                    
   

out show dl <dl-uuid>
=====================

::

   pluto*CLI> help out show dl 
   Usage: out show dl <dl-uuid>
         Show detail given dl info.

::

   pluto*CLI> out show dl f9dfa7d2-5223-4b63-aca1-881ebacae420
   Dl detail info. dl-uuid[f9dfa7d2-5223-4b63-aca1-881ebacae420]
   
   {
     "uuid": "f9dfa7d2-5223-4b63-aca1-881ebacae420",
     "dlma_uuid": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
     "in_use": 1,
     "name": "client 01",
     "detail": "Dial to client 01",
     "status": 0,
     "resv_target": null,
     "ukey": null,
     "udata": null,
     "dialing_uuid": null,
     "dialing_camp_uuid": null,
     "dialing_plan_uuid": null,
     "number_1": "300",
     "number_2": null,
     "number_3": null,
     "number_4": null,
     "number_5": null,
     "number_6": null,
     "number_7": null,
     "number_8": null,
     "email": null,
     "trycnt_1": 0,
     "trycnt_2": 0,
     "trycnt_3": 0,
     "trycnt_4": 0,
     "trycnt_5": 0,
     "trycnt_6": 0,
     "trycnt_7": 0,
     "trycnt_8": 0,
     "res_dial": 0,
     "res_dial_detail": null,
     "res_hangup": 0,
     "res_hangup_detail": null,
     "tm_create": "2016-10-22T13:47:23.899652156Z",
     "tm_delete": null,
     "tm_update": null,
     "tm_last_dial": null
   }
   
out show destinations
=====================

::

   pluto*CLI> help out show destinations
   Usage: out show destinations
         Lists all registered destinations.

Example
-------

::

   pluto*CLI> out show destinations
   Uuid                                 Name       Detail               Type  Exten      Context    Applicatio Data      
   4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94 destinatio test destination         1                       park                 


out show destination <dest-uuid>
================================

::

   pluto*CLI> help out show destination 
   Usage: out show destination <dest-uuid>
         Show detail given destination info.


Example
-------

::

   pluto*CLI> out show destination 4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94
   Destination detail info. dest-uuid[4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94]
   
   {
     "uuid": "4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94",
     "name": "destination test",
     "detail": "test destination",
     "in_use": 1,
     "type": 1,
     "exten": null,
     "context": null,
     "priority": null,
     "variables": null,
     "application": "park",
     "data": null,
     "tm_create": "2016-10-22T14:14:26.443747068Z",
     "tm_delete": null,
     "tm_update": null
   }


out show campaigns
==================

::

   pluto*CLI> help out show campaigns
   Usage: out show campaigns
         Lists all currently registered campaigns.

Example
-------

::

   pluto*CLI> out show campaigns
   Uuid                                 Name       Detail               Status     Plan       Dlma       Dest      
   02c4aebf-789c-46aa-817e-b7406416d211 Sales camp test campaign                 0 4ea35c4b-c acc994d2-0 4e6ed9e6-5
   

out show campaign <camp-uuid>
=============================

::

   pluto*CLI> help out show campaign
   Usage: out show campaign <camp-uuid>
         Show detail given campaign info.


Example
-------

::

   pluto*CLI> out show campaign 02c4aebf-789c-46aa-817e-b7406416d211
   Campaign detail info. camp-uuid[02c4aebf-789c-46aa-817e-b7406416d211]
   
   {
     "uuid": "02c4aebf-789c-46aa-817e-b7406416d211",
     "detail": "test campaign",
     "name": "Sales campaign",
     "status": 0,
     "in_use": 1,
     "next_campaign": null,
     "plan": "4ea35c4b-c2db-4a22-baef-443b5fadd677",
     "dlma": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
     "dest": "4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94",
     "tm_create": "2016-10-22T14:34:45.33929956Z",
     "tm_delete": null,
     "tm_update": null
   }

out show dialings
=================

::

   pluto*CLI> help out show dialings
   Usage: out show dialings
         Lists all currently on service dialings.


Example
-------

::

   pluto*CLI> out show dialings
   Uuid                                 Status Channel              Addr                 Camp       Plan       Dlma       Dest       Dl        
   73dfbf10-8455-48e1-84b8-190b6f63cb11      1 sip/300              300                  02c4aebf-7 4ea35c4b-c acc994d2-0 4e6ed9e6-5 7d545ae0-d
   ad0a2345-08e7-4cec-bdce-36d9c4746e1a      1 sip/300              300                  02c4aebf-7 4ea35c4b-c acc994d2-0 4e6ed9e6-5 f9dfa7d2-5

out show dialing <dialing-uuid>
===============================

::

   pluto*CLI> help out show dialing
   Usage: out show dialing <dialing-uuid>
         Show detail given dialing info.


Example
-------

::

   pluto*CLI> out show dialing ad0a2345-08e7-4cec-bdce-36d9c4746e1a
   Dialing detail info. dialing-uuid[ad0a2345-08e7-4cec-bdce-36d9c4746e1a]
   
   {
     "uuid": "ad0a2345-08e7-4cec-bdce-36d9c4746e1a",
     "status": 1,
     "name": "SIP/300-0000000a",
     "tm_create": "2016-10-22T15:30:27.793155560Z",
     "tm_update": "2016-10-22T15:30:27.799098693Z",
     "tm_delete": "",
     "j_dialing": {
       "dialing_uuid": "ad0a2345-08e7-4cec-bdce-36d9c4746e1a",
       "camp_uuid": "02c4aebf-789c-46aa-817e-b7406416d211",
       "plan_uuid": "4ea35c4b-c2db-4a22-baef-443b5fadd677",
       "dlma_uuid": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
       "dest_uuid": "4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94",
       "dl_list_uuid": "f9dfa7d2-5223-4b63-aca1-881ebacae420",
       "info_camp": {
         "uuid": "02c4aebf-789c-46aa-817e-b7406416d211",
         "name": "Sales campaign",
         "detail": "test campaign",
         "status": 1,
         "in_use": 1,
         "next_campaign": null,
         "tm_delete": null,
         "dlma": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
         "plan": "4ea35c4b-c2db-4a22-baef-443b5fadd677",
         "tm_create": "2016-10-22T14:34:45.33929956Z",
         "dest": "4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94",
         "tm_update": "2016-10-22T15:30:21.705707350Z"
       },
       "info_plan": {
         "uuid": "4ea35c4b-c2db-4a22-baef-443b5fadd677",
         "detail": "simple sales plan",
         "tech_name": "sip/",
         "name": "sales_plan",
         "uui_field": null,
         "in_use": 1,
         "max_retry_cnt_2": 5,
         "max_retry_cnt_8": 5,
         "max_retry_cnt_7": 5,
         "dial_mode": 1,
         "caller_id": null,
         "max_retry_cnt_1": 5,
         "dial_timeout": 30000,
         "retry_delay": 50000,
         "max_retry_cnt_5": 5,
         "dl_end_handle": 1,
         "max_retry_cnt_3": 5,
         "trunk_name": null,
         "tm_update": null,
         "service_level": 0,
         "tm_delete": null,
         "max_retry_cnt_4": 5,
         "max_retry_cnt_6": 5,
         "tm_create": "2016-10-22T12:45:58.868877001Z"
       },
       "info_dlma": {
         "tm_update": null,
         "uuid": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
         "dl_table": "acc994d2_04d9_4a53_bfcf_50c96ff924bc",
         "detail": "Test Dlma description",
         "name": "DialListMaster_Sales",
         "in_use": 1,
         "tm_create": "2016-10-22T13:29:35.820096127Z",
         "tm_delete": null
       },
       "info_dest": {
         "tm_update": null,
         "context": null,
         "application": "park",
         "uuid": "4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94",
         "detail": "test destination",
         "variables": null,
         "name": "destination test",
         "in_use": 1,
         "priority": null,
         "tm_create": "2016-10-22T14:14:26.443747068Z",
         "type": 1,
         "exten": null,
         "data": null,
         "tm_delete": null
       },
       "info_dl_list": {
         "dialing_uuid": null,
         "tm_update": null,
         "ukey": null,
         "email": null,
         "uuid": "f9dfa7d2-5223-4b63-aca1-881ebacae420",
         "number_1": "300",
         "dialing_plan_uuid": null,
         "dlma_uuid": "acc994d2-04d9-4a53-bfcf-50c96ff924bc",
         "in_use": 1,
         "name": "client 01",
         "trycnt_5": 0,
         "number_6": null,
         "res_dial_detail": null,
         "resv_target": null,
         "number_3": null,
         "detail": "Dial to client 01",
         "status": 0,
         "trycnt_7": 0,
         "res_dial": 5,
         "number_2": null,
         "udata": null,
         "trycnt": 3,
         "trycnt_3": 0,
         "dialing_camp_uuid": null,
         "number_4": null,
         "res_hangup": 17,
         "number_5": null,
         "number_7": null,
         "number_8": null,
         "trycnt_8": 0,
         "trycnt_1": 3,
         "trycnt_2": 0,
         "trycnt_4": 0,
         "tm_last_dial": "2016-10-22T15:28:08.232616848Z",
         "trycnt_6": 0,
         "res_hangup_detail": null,
         "tm_create": "2016-10-22T13:47:23.899652156Z",
         "tm_delete": null
       },
       "info_dial": {
         "dial_application": "park",
         "dial_data": "",
         "dial_type": 1,
         "timeout": 30000,
         "dial_index": 1,
         "uuid": "f9dfa7d2-5223-4b63-aca1-881ebacae420",
         "dial_addr": "300",
         "dial_channel": "sip/300",
         "dial_trycnt": 4,
         "channelid": "ad0a2345-08e7-4cec-bdce-36d9c4746e1a",
         "otherchannelid": "3702cb20-4eb0-40a7-926b-bbcde07451a9"
       },
       "dial_channel": "sip/300",
       "dial_addr": "300",
       "dial_index": 1,
       "dial_trycnt": 4,
       "dial_type": 1,
       "dial_application": "park",
       "dial_data": "",
       "tm_dialing": "2016-10-22T15:30:27.793155560Z",
       "channel_name": "SIP/300-0000000a"
     },
     "j_event": {
       "connectedlinenum": "<unknown>",
       "calleridname": "<unknown>",
       "event": "Newstate",
       "privilege": "call,all",
       "channelstate": "5",
       "channel": "SIP/300-0000000a",
       "channelstatedesc": "Ringing",
       "calleridnum": "<unknown>",
       "connectedlinename": "<unknown>",
       "linkedid": "ad0a2345-08e7-4cec-bdce-36d9c4746e1a",
       "language": "en",
       "accountcode": "",
       "context": "public",
       "exten": "",
       "priority": "1",
       "uniqueid": "ad0a2345-08e7-4cec-bdce-36d9c4746e1a",
       "tm_event": "2016-10-22T15:30:27.864585447Z"
     }
   }

