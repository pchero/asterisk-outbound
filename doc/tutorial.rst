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
   QueueName: sales_1
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

