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

Create a queue
--------------
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
   Secret: ****
   
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
   

Create Destination
------------------
::

   Action: OutDestinationCreate
   Name: destination test
   Detail: test destination
   Type: 1
   Application: park
   
   Response: Success
   Message: Dl list created successfully


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

 
Normal call distribute
======================

Dial to the customer. After the customer answered call, the call will be distributed to the waiting agents.


Create a queue
--------------
Assume that we have a below queue info.

::

   /etc/asterisk/queues.conf

   [sales_1]
   musicclass = default
   strategy = ringall
   joinempty = yes


Add members
-----------

Create plan
-----------

Create destination
------------------

Create dlma and dial list
-------------------------

Create campaign and status update
---------------------------------

Check result
------------

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

Create plan
-----------
Set dialplan context, extension.

Create destination
------------------

Create dlma and dial list
-------------------------

Create campaign and status update
---------------------------------

Check result
------------

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

