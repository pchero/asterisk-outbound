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
   
Add the client(sip) info
------------------------
Add the client info to the /etc/asterisk/sip.conf

This sip info is act like an client(customer).

Assume that we have a below client info.

::

   [client-01]
   type=friend
   secret=6d1a5096-89b1-11e6-b0fc-d30be6e4489f
   host=dynamic
   
   [client-02]
   type=friend
   secret=8fd6f80a-89b1-11e6-9839-fb2a82d0d524
   host=dynamic
   
   [client-03]
   type=friend
   secret=946e8c20-89b1-11e6-80d6-f3e7376cf70f
   host=dynamic

Create a queue
--------------d
Assume that we have a below queue info.

::

   [sales_1]
   musicclass = default
   strategy = ringall
   joinempty = yes
   

AMI actions
===========

Login
-----

::

   Connection closed by foreign host.
   $ telnet localhost 5038
   Trying 127.0.0.1...
   Connected to localhost.
   Escape character is '^]'.
   Asterisk Call Manager/2.8.0
   
   Action: Login
   Username: admin
   Secret: admin
   
   Response: Success
   Message: Authentication accepted
   

Create Plan
-----------

::

   Action: OutPlanCreate
   Name: sales_plan
   Detail: simple sales plan
   DialMode: 1
   QueueName: sales_1
   TechName: sip/
   
   Response: Success
   Message: Plan created successfully
   
   Event: OutPlanCreate
   Privilege: message,all
   Uuid: d1306a32-0ec6-4389-a5f1-118153b6266e
   Name: sales_plan
   Detail: simple sales plan
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   AnswerHandle: 0
   DlEndHandle: 1
   RetryDelay: 50000
   TrunkName: <unknown>
   TechName: sip/
   QueueName: sales_1
   AmdMode: 0
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-10-04 22:40:41.573445
   TmDelete: <unknown>
   TmUpdate: <unknown>
   

Create Dlma
-----------

::

   Action: OutDlmaCreate
   Name: DialListMaster_Sales
   Detail: Test Dlma description
   
   Response: Success
   Message: Dlma created successfully
   
   Event: OutDlmaCreate
   Privilege: message,all
   Uuid: 9155a63e-6577-4abc-96e4-2c7811b5f639
   Name: DialListMaster_Sales
   Detail: Test Dlma description
   DlTable: 9155a63e_6577_4abc_96e4_2c7811b5f639
   TmCreate: 2016-10-04 22:41:07.528126
   TmDelete: <unknown>
   TmUpdate: <unknown>


Create Dl list
--------------
::

   Action: OutDlListCreate
   DlmaUuid: 9155a63e-6577-4abc-96e4-2c7811b5f639
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
   Plan: d1306a32-0ec6-4389-a5f1-118153b6266e
   Dlma: 9155a63e-6577-4abc-96e4-2c7811b5f639
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: a3e95af9-5ffa-4f53-8908-6095ff24945c
   Name: Sales campaign
   Detail: test campaign
   Status: 0
   Plan: d1306a32-0ec6-4389-a5f1-118153b6266e
   Dlma: 9155a63e-6577-4abc-96e4-2c7811b5f639
   TmCreate: 2016-10-04 22:43:22.899115
   TmDelete: <unknown>
   TmUpdate: <unknown>

   
