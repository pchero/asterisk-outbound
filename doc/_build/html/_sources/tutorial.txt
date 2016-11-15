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
   Uuid: f9866959-9cf5-49fb-aee0-878bfc7bd71f
   Name: sales_plan
   Detail: simple sales plan
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   DlEndHandle: 1
   RetryDelay: 60
   TrunkName: <unknown>
   TechName: sip/
   Variable: <unknown>
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-11-15T02:10:52.930031700Z
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
   Uuid: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   Name: DialListMaster_Sales
   Detail: Test Dlma description
   DlTable: 5d56b51d_dc4d_4ec9_9e82_88e8dc3737c1
   Variable: <unknown>
   TmCreate: 2016-11-15T02:11:22.128567345Z
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
   Uuid: a1c8d1f5-c68f-4a01-8d93-5d110ea654de
   Name: destination test
   Detail: test destination
   Type: 1
   Exten: <unknown>
   Context: <unknown>
   Priority: <unknown>
   Variable: <unknown>
   Application: park
   Data: <unknown>
   TmCreate: 2016-11-15T02:11:43.506539333Z
   TmDelete: <unknown>
   TmUpdate: <unknown>



Create dial list
----------------

::

   Action: OutDlListCreate
   DlmaUuid: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
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
   Plan: f9866959-9cf5-49fb-aee0-878bfc7bd71f
   Dlma: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   Dest: a1c8d1f5-c68f-4a01-8d93-5d110ea654de
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: a438f08f-96a0-4a72-b55d-cb6a74fd91a1
   Name: Sales campaign
   Detail: test campaign
   Status: 0
   Plan: f9866959-9cf5-49fb-aee0-878bfc7bd71f
   Dlma: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   Dest: a1c8d1f5-c68f-4a01-8d93-5d110ea654de
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-15T02:13:09.876226960Z
   TmDelete: <unknown>
   TmUpdate: <unknown>

Update Campaign status to start
-------------------------------

::

   Action: OutCampaignUpdate
   Uuid: a438f08f-96a0-4a72-b55d-cb6a74fd91a1
   Status: 1
   
   Response: Success
   Message: Campaign updated successfully
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: a438f08f-96a0-4a72-b55d-cb6a74fd91a1
   Name: Sales campaign
   Detail: test campaign
   Status: 11
   Plan: f9866959-9cf5-49fb-aee0-878bfc7bd71f
   Dlma: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   Dest: a1c8d1f5-c68f-4a01-8d93-5d110ea654de
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-15T02:13:09.876226960Z
   TmDelete: <unknown>
   TmUpdate: 2016-11-15T03:00:10.125931679Z

   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: a438f08f-96a0-4a72-b55d-cb6a74fd91a1
   Name: Sales campaign
   Detail: test campaign
   Status: 1
   Plan: f9866959-9cf5-49fb-aee0-878bfc7bd71f
   Dlma: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   Dest: a1c8d1f5-c68f-4a01-8d93-5d110ea654de
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-15T02:13:09.876226960Z
   TmDelete: <unknown>
   TmUpdate: 2016-11-15T03:00:12.181524802Z
   
   
   Event: OutDialingCreate
   Privilege: message,all
   Uuid: d070a649-a8e0-43f1-9cf6-671cb4046016
   Status: 0
   CampUuid: a438f08f-96a0-4a72-b55d-cb6a74fd91a1
   PlanUuid: f9866959-9cf5-49fb-aee0-878bfc7bd71f
   DlmaUuid: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   DestUuid: a1c8d1f5-c68f-4a01-8d93-5d110ea654de
   DlListUuid: 8e02be1f-cf5e-424e-8d00-001b2f9935aa
   DialIndex: 1
   DialAddr: 300
   DialChannel: sip/300
   DialTryCnt: 1
   DialTimeout: 30000
   DialType: 1
   DialExten: <unknown>
   DialContext: <unknown>
   DialApplication: park
   DialData: 
   Variable: <unknown>
   ChannelName: <unknown>
   ResDial: 0
   ResHangup: 0
   ResHangupDetail: <unknown>
   TmCreate: 2016-11-15T03:00:12.580796983Z
   TmUpdate: <unknown>
   TmDelete: <unknown>


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
   Uuid: 947da8c6-88a1-4da2-a804-e4570e92fba5
   Name: queue distribute plan
   Detail: simple queue distbute plan
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   DlEndHandle: 1
   RetryDelay: 60
   TrunkName: <unknown>
   TechName: sip/
   Variable: <unknown>
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-11-15T03:16:23.265536071Z
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
   Uuid: 45122654-5633-4af0-a739-e32eddfbd2ae
   Name: destination test
   Detail: test destination
   Type: 1
   Exten: <unknown>
   Context: <unknown>
   Priority: <unknown>
   Variable: <unknown>
   Application: queue
   Data: sales_1
   TmCreate: 2016-11-15T03:17:11.997148863Z
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
   Uuid: bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0
   Name: DialListMaster queue distribute
   Detail: Test Dlma description
   DlTable: bd62639a_3cbb_4fb5_9a2b_e5cdf0c336d0
   Variable: <unknown>
   TmCreate: 2016-11-15T03:17:46.927966757Z
   TmDelete: <unknown>
   TmUpdate: <unknown>

Create dial list(dl)

::

   Action: OutDlListCreate
   DlmaUuid: bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0
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
   Plan: 947da8c6-88a1-4da2-a804-e4570e92fba5
   Dlma: bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0
   Dest: 45122654-5633-4af0-a739-e32eddfbd2ae
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: 665a54d8-f672-48bd-8f05-1163e6b4dc7f
   Name: Sales campaign
   Detail: test campaign
   Status: 0
   Plan: 947da8c6-88a1-4da2-a804-e4570e92fba5
   Dlma: bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0
   Dest: 45122654-5633-4af0-a739-e32eddfbd2ae
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-15T03:20:00.77987326Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


Update campaign status.

::

   Action: OutCampaignUpdate
   Uuid: 665a54d8-f672-48bd-8f05-1163e6b4dc7f
   Status: 1
   
   Response: Success
   Message: Campaign updated successfully
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: 665a54d8-f672-48bd-8f05-1163e6b4dc7f
   Name: Sales campaign
   Detail: test campaign
   Status: 11
   Plan: 947da8c6-88a1-4da2-a804-e4570e92fba5
   Dlma: bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0
   Dest: 45122654-5633-4af0-a739-e32eddfbd2ae
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-15T03:20:00.77987326Z
   TmDelete: <unknown>
   TmUpdate: 2016-11-15T03:20:47.283063453Z
   
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: 665a54d8-f672-48bd-8f05-1163e6b4dc7f
   Name: Sales campaign
   Detail: test campaign
   Status: 1
   Plan: 947da8c6-88a1-4da2-a804-e4570e92fba5
   Dlma: bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0
   Dest: 45122654-5633-4af0-a739-e32eddfbd2ae
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-15T03:20:00.77987326Z
   TmDelete: <unknown>
   TmUpdate: 2016-11-15T03:20:48.181543776Z


Check result
------------

::

   tail -n 1 /var/lib/asterisk/astout.result

   {
       "camp_uuid": "665a54d8-f672-48bd-8f05-1163e6b4dc7f",
       "channel_name": "SIP/300-00000004",
       "channelid": "ff31ef95-30ed-4713-9ae8-0b009b745183",
       "dest_uuid": "45122654-5633-4af0-a739-e32eddfbd2ae",
       "dest_variables": "",
       "dial_addr": "300",
       "dial_application": "queue",
       "dial_channel": "sip/300",
       "dial_data": "sales_1",
       "dial_index": 1,
       "dial_timeout": 30000,
       "dial_trycnt": 1,
       "dial_type": 1,
       "dialing_uuid": "ff31ef95-30ed-4713-9ae8-0b009b745183",
       "dl_list_uuid": "0686b654-16c0-4d1e-bdba-4a295c0d3434",
       "dl_variables": "",
       "dlma_uuid": "bd62639a-3cbb-4fb5-9a2b-e5cdf0c336d0",
       "otherchannelid": "07954c96-9b3a-445c-a6d6-cd34b09af83e",
       "plan_uuid": "947da8c6-88a1-4da2-a804-e4570e92fba5",
       "plan_variables": "",
       "res_dial": 4,
       "res_hangup": 16,
       "res_hangup_detail": "Normal Clearing",
       "tm_dial_begin": "2016-11-15T04:32:24.360896466Z",
       "tm_dial_end": "2016-11-15T04:32:27.686378516Z",
       "tm_dialing": "2016-11-15T04:32:20.791410475Z",
       "tm_hangup": "2016-11-15T04:32:33.257211878Z",
       "uuid": "0686b654-16c0-4d1e-bdba-4a295c0d3434",
       "variables": "{}"
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


Sending an SMS/Fax/Email
========================
Using variables and dialplan, we can send an SMS/Fax/Email to the customers.

Setting dialplan
----------------

Create plan
-----------

Create destination
------------------

Create dlma and dial list
-------------------------

Check result
------------



