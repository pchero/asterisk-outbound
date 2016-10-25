.. tutorial

********
Tutorial
********

Preparation
===========

Add the agent(sip) info
-----------------------
Add the agent info to the /etc/asterisk/sip.conf

This sip info is act like an agent.

Assume that we have below agent info.

::
 
   [agent-01]
   type=friend
   secret=83f8c590-89b1-11e6-9d93-eba8dc621725
   host=dynamic
   
   [agent-02]
   type=friend
   secret=876054be-89b1-11e6-b7b7-cfb4ed95428e
   host=dynamic
   
   [agent-03]
   type=friend
   secret=8b14c86a-89b1-11e6-aa09-f39f39e5592b
   host=dynamic
   
Add the client/customer(sip) info
---------------------------------
Add the client info to the /etc/asterisk/sip.conf

This sip info is act like an client(customer).

Assume that we have a below client info.

::

   [300]
   type=friend
   secret=beb398d1-bf46-4895-a703-544679423e58
   host=dynamic
   
   [301]
   type=friend
   secret=573958ec-862c-4ba3-b73d-3c281277f551
   host=dynamic
   
   [302]
   type=friend
   secret=c3c3b964-cf44-421b-9529-4cb9f0b3e277
   host=dynamic


Settings
--------

::

   pluto*CLI> sip show peers
   Name/username             Host                                    Dyn Forcerport Comedia    ACL Port     Status      Description                      
   300/300                   192.168.118.100                          D  Auto (No)  No             5060     Unmonitored                                  
   301/301                   192.168.118.100                          D  Auto (No)  No             5060     Unmonitored                                  
   302/302                   192.168.118.100                          D  Auto (No)  No             5060     Unmonitored                                  
   agent-01/agent-01         192.168.118.100                          D  Auto (No)  No             5060     Unmonitored                                  
   agent-02/agent-02         192.168.118.100                          D  Auto (No)  No             5060     Unmonitored                                  
   agent-03/agent-03         192.168.118.100                          D  Auto (No)  No             5060     Unmonitored                                  
   6  sip peers [Monitored: 0 online, 0 offline Unmonitored: 6 online, 0 offline]

   
Basic tutorial
==============
Dial to the customer. After the customer answered call, the call will be transfferred to parking lot.

Create plan
-----------

::

   Action: OutPlanCreate
   Name: sales_plan
   Detail: simple sales plan
   DialMode: 1
   TechName: sip/
   
   Response: Success
   Message: Plan created successfully
   
   Event: OutPlanCreate
   Privilege: message,all
   Uuid: 687763ed-c977-4e5b-a30d-1221941a4d21
   Name: sales_plan
   Detail: simple sales plan
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   DlEndHandle: 1
   RetryDelay: 60
   TrunkName: <unknown>
   TechName: sip/
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-10-24T21:36:03.702384603Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create dlma
-----------

::

   Action: OutDlmaCreate
   Name: DialListMaster_Sales
   Detail: Test Dlma description
   
   Response: Success
   Message: Dlma created successfully
   
   Event: OutDlmaCreate
   Privilege: message,all
   Uuid: a6a29e7a-49c4-4339-92ff-543a121f348f
   Name: DialListMaster_Sales
   Detail: Test Dlma description
   DlTable: a6a29e7a_49c4_4339_92ff_543a121f348f
   TmCreate: 2016-10-24T21:37:39.972064103Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create destination
------------------

::

   Action: OutDestinationCreate
   Name: destination test
   Detail: test destination
   Type: 1
   Application: park
   
   Response: Success
   Message: Destination created successfully
   
   Event: OutDestinationCreate
   Privilege: message,all
   Uuid: ef355147-48bf-4170-8f88-f49b00f3ab37
   Name: destination test
   Detail: test destination
   Type: 1
   Exten: <unknown>
   Context: <unknown>
   Priority: <unknown>
   Variable: <unknown>
   Application: park
   Data: <unknown>
   TmCreate: 2016-10-24T21:38:35.700905321Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create dial list
----------------

::

   Action: OutDlListCreate
   DlmaUuid: a6a29e7a-49c4-4339-92ff-543a121f348f
   Name: client 01
   Detail: Dial to client 01
   Number1: 300
   
   Response: Success
   Message: Dl list created successfully


Create campaign
---------------

::

   Action: OutCampaignCreate
   Name: Sales campaign
   Detail: test campaign
   Plan: 687763ed-c977-4e5b-a30d-1221941a4d21
   Dlma: a6a29e7a-49c4-4339-92ff-543a121f348f
   Dest: ef355147-48bf-4170-8f88-f49b00f3ab37
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: c429c3cc-265f-458a-b64f-30023d4896d4
   Name: Sales campaign
   Detail: test campaign
   Status: 0
   Plan: 687763ed-c977-4e5b-a30d-1221941a4d21
   Dlma: a6a29e7a-49c4-4339-92ff-543a121f348f
   Dest: ef355147-48bf-4170-8f88-f49b00f3ab37
   TmCreate: 2016-10-24T21:41:24.939663006Z
   TmDelete: <unknown>
   TmUpdate: <unknown>

Update Campaign status to start
-------------------------------

::

   Action: OutCampaignUpdate
   Uuid: c429c3cc-265f-458a-b64f-30023d4896d4
   Status: 1
   
   Response: Success
   Message: Campaign updated successfully
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: c429c3cc-265f-458a-b64f-30023d4896d4
   Name: Sales campaign
   Detail: test campaign
   Status: 1
   Plan: 687763ed-c977-4e5b-a30d-1221941a4d21
   Dlma: a6a29e7a-49c4-4339-92ff-543a121f348f
   Dest: ef355147-48bf-4170-8f88-f49b00f3ab37
   TmCreate: 2016-10-24T21:41:24.939663006Z
   TmDelete: <unknown>
   TmUpdate: 2016-10-24T21:42:38.66081477Z

 
Normal call distribute
======================

Dial to the customer. After the customer answered call, the call will be distributed to the waiting agents.


Create a queue
--------------
Add the queue info to the /etc/asterisk/queues.conf

Assume that we have a below queue info.

::

   [sales_1]
   musicclass = default
   strategy = ringall
   joinempty = yes


Add members
-----------
Add the all agents to the queue.

::

   pluto*CLI> queue add member sip/agent-01 to sales_1 
   Added interface 'sip/agent-01' to queue 'sales_1'
   
   pluto*CLI> queue add member sip/agent-02 to sales_1
   Added interface 'sip/agent-02' to queue 'sales_1'
   
   pluto*CLI> queue add member sip/agent-03 to sales_1
   Added interface 'sip/agent-03' to queue 'sales_1'


Create plan
-----------

::

   Action: OutPlanCreate
   Name: queue distribute plan
   Detail: simple queue distbute plan
   DialMode: 1
   TechName: sip/
   
   Response: Success
   Message: Plan created successfully
   
   Event: OutPlanCreate
   Privilege: message,all
   Uuid: 5acea376-195a-4519-b68f-58e9ceaadc68
   Name: queue distribute plan
   Detail: simple queue distbute plan
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   DlEndHandle: 1
   RetryDelay: 60
   TrunkName: <unknown>
   TechName: sip/
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-10-24T22:46:14.893825038Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create destination
------------------

::

   Action: OutDestinationCreate
   Name: destination test
   Detail: test destination
   Type: 1
   Application: queue
   Data: sales_1
   
   Response: Success
   Message: Destination created successfully
   
   Event: OutDestinationCreate
   Privilege: message,all
   Uuid: 1a88f58d-3353-4a55-83be-1d6ab58b2bfc
   Name: destination test
   Detail: test destination
   Type: 1
   Exten: <unknown>
   Context: <unknown>
   Priority: <unknown>
   Variable: <unknown>
   Application: queue
   Data: sales_1
   TmCreate: 2016-10-24T22:48:11.604966289Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create dlma and dial list
-------------------------

Create Dlma

::

   Action: OutDlmaCreate
   Name: DialListMaster queue distribute
   Detail: Test Dlma description
   
   Response: Success
   Message: Dlma created successfully
   
   Event: OutDlmaCreate
   Privilege: message,all
   Uuid: 8f1cda4d-1a95-4cbc-9865-fb604ce3f70a
   Name: DialListMaster queue distribute
   Detail: Test Dlma description
   DlTable: 8f1cda4d_1a95_4cbc_9865_fb604ce3f70a
   TmCreate: 2016-10-24T22:47:00.685610240Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create dial list(dl)

::

   Action: OutDlListCreate
   DlmaUuid: 8f1cda4d-1a95-4cbc-9865-fb604ce3f70a
   Name: client 01
   Detail: Dial to client 01
   Number1: 300
   
   Response: Success
   Message: Dl list created successfully


Create campaign and status update
---------------------------------

Create campaign.

::

   Action: OutCampaignCreate
   Name: Sales campaign
   Detail: test campaign
   Plan: 5acea376-195a-4519-b68f-58e9ceaadc68
   Dlma: 8f1cda4d-1a95-4cbc-9865-fb604ce3f70a
   Dest: 1a88f58d-3353-4a55-83be-1d6ab58b2bfc
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: ea289ed8-92f3-430c-b00c-b5254257282b
   Name: Sales campaign
   Detail: test campaign
   Status: 0
   Plan: 5acea376-195a-4519-b68f-58e9ceaadc68
   Dlma: 8f1cda4d-1a95-4cbc-9865-fb604ce3f70a
   Dest: 1a88f58d-3353-4a55-83be-1d6ab58b2bfc
   TmCreate: 2016-10-24T22:49:45.907295315Z
   TmDelete: <unknown>
   TmUpdate: <unknown>

Update campaign status.

::

   Action: OutCampaignUpdate
   Uuid: ea289ed8-92f3-430c-b00c-b5254257282b
   Status: 1
   
   Response: Success
   Message: Campaign updated successfully
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: ea289ed8-92f3-430c-b00c-b5254257282b
   Name: Sales campaign
   Detail: test campaign
   Status: 1
   Plan: 5acea376-195a-4519-b68f-58e9ceaadc68
   Dlma: 8f1cda4d-1a95-4cbc-9865-fb604ce3f70a
   Dest: 1a88f58d-3353-4a55-83be-1d6ab58b2bfc
   TmCreate: 2016-10-24T22:49:45.907295315Z
   TmDelete: <unknown>
   TmUpdate: 2016-10-24T22:52:16.250101358Z



Check result
------------

::

   tail -n 1 /var/lib/asterisk/astout.result

   {
      "dialing_uuid": "a624ecec-e3a8-4e95-9538-abed6e2271ab",
      "camp_uuid": "ea289ed8-92f3-430c-b00c-b5254257282b",
      "plan_uuid": "5acea376-195a-4519-b68f-58e9ceaadc68",
      "tm_hangup": "2016-10-24T22:51:32.482367256Z",
      "dlma_uuid": "8f1cda4d-1a95-4cbc-9865-fb604ce3f70a",
      "channel_name": "SIP/300-00000014",
      "tm_dial_begin": "2016-10-24T22:51:27.734721762Z",
      "info_camp": {
         "uuid": "ea289ed8-92f3-430c-b00c-b5254257282b",
         "plan": "5acea376-195a-4519-b68f-58e9ceaadc68",
         "dlma": "8f1cda4d-1a95-4cbc-9865-fb604ce3f70a",
         "detail": "test campaign",
         "name": "Sales campaign",
         "status": 1,
         "in_use": 1,
         "next_campaign": null,
         "dest": "1a88f58d-3353-4a55-83be-1d6ab58b2bfc",
         "tm_create": "2016-10-24T22:49:45.907295315Z",
         "tm_delete": null,
         "tm_update": "2016-10-24T22:50:10.706866142Z"
      },
      "dest_uuid": "1a88f58d-3353-4a55-83be-1d6ab58b2bfc",
      "res_hangup_detail": "Normal Clearing",
      "dial_addr": "300",
      "dl_list_uuid": "8e0d1ef2-faf0-42d8-a70a-b494cae7f90d",
      "info_plan": {
         "caller_id": null,
         "uuid": "5acea376-195a-4519-b68f-58e9ceaadc68",
         "trunk_name": null,
         "dl_end_handle": 1,
         "detail": "simple queue distbute plan",
         "name": "queue distribute plan",
         "max_retry_cnt_2": 5,
         "max_retry_cnt_5": 5,
         "uui_field": null,
         "tm_update": null,
         "service_level": 0,
         "in_use": 1,
         "dial_mode": 1,
         "retry_delay": 60,
         "max_retry_cnt_6": 5,
         "dial_timeout": 30000,
         "tech_name": "sip/",
         "max_retry_cnt_1": 5,
         "max_retry_cnt_3": 5,
         "max_retry_cnt_4": 5,
         "max_retry_cnt_7": 5,
         "max_retry_cnt_8": 5,
         "tm_create": "2016-10-24T22:46:14.893825038Z",
         "tm_delete": null
      },
      "info_dlma": {
         "uuid": "8f1cda4d-1a95-4cbc-9865-fb604ce3f70a",
         "detail": "Test Dlma description",
         "name": "DialListMaster queue distribute",
         "dl_table": "8f1cda4d_1a95_4cbc_9865_fb604ce3f70a",
         "tm_update": null,
         "in_use": 1,
         "tm_create": "2016-10-24T22:47:00.685610240Z",
         "tm_delete": null
      },
      "info_dest": {
         "uuid": "1a88f58d-3353-4a55-83be-1d6ab58b2bfc",
         "name": "destination test",
         "detail": "test destination",
         "in_use": 1,
         "type": 1,
         "exten": null,
         "context": null,
         "tm_create": "2016-10-24T22:48:11.604966289Z",
         "application": "queue",
         "priority": null,
         "variables": null,
         "tm_update": null,
         "data": "sales_1",
         "tm_delete": null
      },
      "dial_trycnt": 1,
      "dial_channel": "sip/300",
      "info_dl_list": {
         "number_4": null,
         "number_8": null,
         "uuid": "8e0d1ef2-faf0-42d8-a70a-b494cae7f90d",
         "number_3": null,
         "ukey": null,
         "tm_update": null,
         "dlma_uuid": "8f1cda4d-1a95-4cbc-9865-fb604ce3f70a",
         "in_use": 1,
         "tm_last_dial": null,
         "detail": "Dial to client 01",
         "name": "client 01",
         "status": 0,
         "dialing_camp_uuid": null,
         "resv_target": null,
         "number_6": null,
         "udata": null,
         "res_hangup_detail": null,
         "dialing_uuid": null,
         "number_2": null,
         "trycnt_4": 0,
         "res_dial_detail": null,
         "dialing_plan_uuid": null,
         "trycnt_3": 0,
         "number_1": "300",
         "number_5": null,
         "trycnt_2": 0,
         "number_7": null,
         "email": null,
         "trycnt_1": 0,
         "trycnt_5": 0,
         "trycnt_6": 0,
         "trycnt_7": 0,
         "trycnt_8": 0,
         "res_dial": 0,
         "res_hangup": 0,
         "tm_create": "2016-10-24T22:48:43.572379619Z",
         "tm_delete": null,
         "tm_last_hangup": null,
         "trycnt": 0
      },
      "dial_index": 1,
      "dial_data": "sales_1",
      "info_dial": {
         "dial_application": "queue",
         "dial_index": 1,
         "dial_data": "sales_1",
         "dial_trycnt": 1,
         "dial_channel": "sip/300",
         "dial_type": 1,
         "uuid": "8e0d1ef2-faf0-42d8-a70a-b494cae7f90d",
         "channelid": "a624ecec-e3a8-4e95-9538-abed6e2271ab",
         "dial_addr": "300",
         "timeout": 30000,
         "otherchannelid": "cb1325bd-4ae7-4db8-aa64-bb0babadb782"
      },
      "dial_type": 1,
      "tm_dialing": "2016-10-24T22:50:10.784443999Z",
      "dial_application": "queue",
      "res_hangup": 16,
      "res_dial": 4,
      "tm_dial_end": "2016-10-24T22:51:29.294001808Z"
   }

Power dialing
=============
Dial to the customer. After the customer answered call, the recorded message will be played.

Create plan
-----------
Set application Playback with data.

Create destination
------------------

Create dlma and dial list
-------------------------

Create campaign and status update
---------------------------------

Check result
------------

Transfer to the dialplan
========================
Dial to the customer. If the customer answered call, the call will be transferred to the designated dialplan.

Sample dialplan
---------------

::
   
   /etc/asterisk/extension.conf

   ;
   ; ANI context: use in the same way as "time" above
   ;
   
   [ani]
   exten => _X.,40000(ani),NoOp(ANI: ${EXTEN})
   exten => _X.,n,Wait(0.25)
   exten => _X.,n,Answer()
   exten => _X.,n,Playback(vm-from)
   exten => _X.,n,SayDigits(${CALLERID(ani)})
   exten => _X.,n,Wait(1.25)
   exten => _X.,n,SayDigits(${CALLERID(ani)})      ; playback again in case of missed digit
   exten => _X.,n,Return()


Create plan
-----------
Set dialplan context, extension.

::

   Action: OutPlanCreate
   Name: test plan
   Detail: extension test plan
   DialMode: 1
   TechName: sip/
   
   Response: Success
   Message: Plan created successfully
   
   Event: OutPlanCreate
   Privilege: message,all
   Uuid: 5b38a18f-ff18-4cb0-97fd-8450f2f77808
   Name: test plan
   Detail: extension test plan
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   DlEndHandle: 1
   RetryDelay: 60
   TrunkName: <unknown>
   TechName: sip/
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-10-25T21:31:02.316412156Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create destination
------------------

::

   Action: OutDestinationCreate
   Name: test destination
   Detail: extension test destination
   Type: 0
   Exten: 10
   Context: ani
   Priority: 40000
   
   Response: Success
   Message: Destination created successfully
   
   Event: OutDestinationCreate
   Privilege: message,all
   Uuid: 7b55762b-c860-46cc-a366-6bf3df46b2f9
   Name: test destination
   Detail: extension test destination
   Type: 0
   Exten: 10
   Context: ani
   Priority: 40000
   Variable: <unknown>
   Application: <unknown>
   Data: <unknown>
   TmCreate: 2016-10-25T21:32:46.433808884Z
   TmDelete: <unknown>
   TmUpdate: <unknown>



Create dlma and dial list
-------------------------

::

   Action: OutDlmaCreate
   Name: DialListMaster_Sales
   Detail: extension Test Dlma description
   
   Response: Success
   Message: Dlma created successfully
   
   Event: OutDlmaCreate
   Privilege: message,all
   Uuid: b0185b80-444a-4def-be23-111f89d444d0
   Name: DialListMaster_Sales
   Detail: extension Test Dlma description
   DlTable: b0185b80_444a_4def_be23_111f89d444d0
   TmCreate: 2016-10-25T21:31:49.411809915Z
   TmDelete: <unknown>
   TmUpdate: <unknown>
   
   Action: OutDlListCreate
   DlmaUuid: b0185b80-444a-4def-be23-111f89d444d0
   Name: client 01
   Detail: Dial to client 01
   Number1: 300
   
   Response: Success
   Message: Dl list created successfully
   


Create campaign and status update
---------------------------------

::

   Action: OutCampaignCreate
   Name: test campaign
   Detail: extension test campaign
   Plan: 5b38a18f-ff18-4cb0-97fd-8450f2f77808
   Dlma: b0185b80-444a-4def-be23-111f89d444d0
   Dest: 7b55762b-c860-46cc-a366-6bf3df46b2f9
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: c8cc2b6f-178e-4118-9c0c-b662811eeba6
   Name: test campaign
   Detail: extension test campaign
   Status: 0
   Plan: 5b38a18f-ff18-4cb0-97fd-8450f2f77808
   Dlma: b0185b80-444a-4def-be23-111f89d444d0
   Dest: 7b55762b-c860-46cc-a366-6bf3df46b2f9
   TmCreate: 2016-10-25T21:33:48.41309734Z
   TmDelete: <unknown>
   TmUpdate: <unknown>
   
   Action: OutCampaignUpdate
   Uuid: c8cc2b6f-178e-4118-9c0c-b662811eeba6
   Status: 1
   
   Response: Success
   Message: Campaign updated successfully
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: c8cc2b6f-178e-4118-9c0c-b662811eeba6
   Name: test campaign
   Detail: extension test campaign
   Status: 1
   Plan: 5b38a18f-ff18-4cb0-97fd-8450f2f77808
   Dlma: b0185b80-444a-4def-be23-111f89d444d0
   Dest: 7b55762b-c860-46cc-a366-6bf3df46b2f9
   TmCreate: 2016-10-25T21:33:48.41309734Z
   TmDelete: <unknown>
   TmUpdate: 2016-10-25T21:35:10.24364534Z
   

Check result
------------

::

   {
      "dialing_uuid": "ed927e7e-6e59-467b-8cb8-0c44da80fa34",
      "dial_exten": "10",
      "info_dlma": {
         "name": "DialListMaster_Sales",
         "uuid": "b0185b80-444a-4def-be23-111f89d444d0",
         "detail": "extension Test Dlma description",
         "dl_table": "b0185b80_444a_4def_be23_111f89d444d0",
         "tm_update": null,
         "in_use": 1,
         "tm_delete": null,
         "tm_create": "2016-10-25T21:31:49.411809915Z"
      },
      "info_plan": {
         "name": "test plan",
         "trunk_name": null,
         "uuid": "5b38a18f-ff18-4cb0-97fd-8450f2f77808",
         "retry_delay": 60,
         "codecs": null,
         "detail": "extension test plan",
         "max_retry_cnt_6": 5,
         "in_use": 1,
         "dial_timeout": 30000,
         "uui_field": null,
         "max_retry_cnt_7": 5,
         "max_retry_cnt_3": 5,
         "dial_mode": 1,
         "service_level": 0,
         "caller_id": null,
         "max_retry_cnt_4": 5,
         "dl_end_handle": 1,
         "tech_name": "sip/",
         "early_media": null,
         "max_retry_cnt_5": 5,
         "max_retry_cnt_1": 5,
         "max_retry_cnt_2": 5,
         "max_retry_cnt_8": 5,
         "tm_create": "2016-10-25T21:31:02.316412156Z",
         "tm_delete": null,
         "tm_update": null
      },
      "dial_timeout": 30000,
      "res_hangup": 16,
      "camp_uuid": "c8cc2b6f-178e-4118-9c0c-b662811eeba6",
      "info_dest": {
         "name": "test destination",
         "priority": "40000",
         "uuid": "7b55762b-c860-46cc-a366-6bf3df46b2f9",
         "detail": "extension test destination",
         "exten": "10",
         "tm_update": null,
         "data": null,
         "in_use": 1,
         "application": null,
         "variables": null,
         "type": 0,
         "context": "ani",
         "tm_create": "2016-10-25T21:32:46.433808884Z",
         "tm_delete": null
      },
      "dlma_uuid": "b0185b80-444a-4def-be23-111f89d444d0",
      "dial_type": 0,
      "dial_addr": "300",
      "dial_priority": "40000",
      "plan_uuid": "5b38a18f-ff18-4cb0-97fd-8450f2f77808",
      "dial_context": "ani",
      "dest_uuid": "7b55762b-c860-46cc-a366-6bf3df46b2f9",
      "dl_list_uuid": "a9b2efb4-9f2e-4728-8b1b-06d9928af157",
      "res_dial": 4,
      "info_camp": {
         "name": "test campaign",
         "status": 1,
         "uuid": "c8cc2b6f-178e-4118-9c0c-b662811eeba6",
         "detail": "extension test campaign",
         "in_use": 1,
         "dlma": "b0185b80-444a-4def-be23-111f89d444d0",
         "next_campaign": null,
         "tm_delete": null,
         "plan": "5b38a18f-ff18-4cb0-97fd-8450f2f77808",
         "dest": "7b55762b-c860-46cc-a366-6bf3df46b2f9",
         "tm_create": "2016-10-25T21:33:48.41309734Z",
         "tm_update": "2016-10-25T21:35:10.24364534Z"
      },
      "info_dl_list": {
         "res_hangup_detail": null,
         "name": "client 01",
         "status": 0,
         "number_4": null,
         "uuid": "a9b2efb4-9f2e-4728-8b1b-06d9928af157",
         "dlma_uuid": "b0185b80-444a-4def-be23-111f89d444d0",
         "in_use": 1,
         "dialing_plan_uuid": null,
         "detail": "Dial to client 01",
         "tm_delete": null,
         "dialing_camp_uuid": null,
         "ukey": null,
         "res_dial_detail": null,
         "tm_last_dial": null,
         "trycnt_4": 0,
         "tm_last_hangup": null,
         "resv_target": null,
         "dialing_uuid": null,
         "udata": null,
         "number_1": "300",
         "number_2": null,
         "trycnt_8": 0,
         "trycnt_7": 0,
         "number_3": null,
         "number_5": null,
         "number_6": null,
         "number_7": null,
         "trycnt_2": 0,
         "number_8": null,
         "trycnt_1": 0,
         "email": null,
         "trycnt_3": 0,
         "trycnt_5": 0,
         "tm_create": "2016-10-25T21:32:23.177900592Z",
         "trycnt_6": 0,
         "res_dial": 0,
         "res_hangup": 0,
         "tm_update": null,
         "trycnt": 0
      },
      "tm_hangup": "2016-10-25T21:35:16.591497163Z",
      "info_dial": {
         "dial_timeout": 30000,
         "dial_exten": "10",
         "dial_priority": "40000",
         "dial_context": "ani",
         "dial_addr": "300",
         "dial_index": 1,
         "dial_type": 0,
         "dial_trycnt": 1,
         "uuid": "a9b2efb4-9f2e-4728-8b1b-06d9928af157",
         "dial_channel": "sip/300",
         "channelid": "ed927e7e-6e59-467b-8cb8-0c44da80fa34",
         "otherchannelid": "0792c7fe-5fb4-4826-9938-306e43bce448"
      },
      "dial_index": 1,
      "dial_trycnt": 1,
      "uuid": "a9b2efb4-9f2e-4728-8b1b-06d9928af157",
      "dial_channel": "sip/300",
      "channelid": "ed927e7e-6e59-467b-8cb8-0c44da80fa34",
      "tm_dial_end": "2016-10-25T21:35:14.333766501Z",
      "info_events": [],
      "otherchannelid": "0792c7fe-5fb4-4826-9938-306e43bce448",
      "tm_dialing": "2016-10-25T21:35:10.462824521Z",
      "channel_name": "SIP/300-00000000",
      "res_hangup_detail": "Normal Clearing"
   }


Transfer to the dialplan check Human/Machine
============================================
Dial to the customer. If the customer answered call, the call will be transferred to the designated dialplan.

Then check the who is answered it(Human/Machine).

Create plan
-----------
Set dialplan context, extension. AMD() application.

Create destination
------------------

Create dlma and dial list
-------------------------

Create campaign and status update
---------------------------------

Check result
------------


Outbound with trunk
===================

Dial to the customer with trunk.

With correct trunk setting, it could call to the PSTN/Mobile.

Set/Check trunk info
--------------------

Create plan
-----------
Set dialplan context, extension, trunk name.

Create destination
------------------

Create dlma and dial list
-------------------------

Create campaign and status update
---------------------------------

Check result
------------



