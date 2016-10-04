.. ami_action

**********
AMI Action
**********

Overview
========
AMI action message for Asterisk-outbound.

OutCampaignCreate
=================
Description
-----------
Create new campaign.

Syntax
------

::

    Action: OutCampaignCreate
    ActionID: <value>
    [Name:] <value>
    [Detail:] <value>
    [Plan:] <value>
    [Dlma:] <value>

Parameters

* ``Name``: Name of campaign.
* ``Detail``: Description of campaign.
* ``Plan``: The UUID of plan.
* ``Dlma``: The UUID of dlma.

Returns
-------
::

    Response: Success
    Message: Campaign created successfully

Example
-------
::

   Action: OutCampaignCreate
   
   Response: Success
   Message: Campaign created successfully
   
   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: 0198236d-0ffd-4c8d-b3fb-133d976f8a01
   Name: <unknown>
   Detail: <unknown>
   Status: 0
   Plan: <unknown>
   Dlma: <unknown>
   TmCreate: 2016-10-03 20:05:49.429024
   TmDelete: <unknown>
   TmUpdate: <unknown>


OutCampaignUpdate
=================
Description
-----------
Update exist campaign info.

Syntax
------

::

    Action: OutCampaignUpdate
    ActionID: <value>
    Uuid: <value>
    [Name:] <value>
    [Detail:] <value>
    [Status:] <value>
    [Plan:] <value>
    [Dlma:] <value>

Parameters

* ``Uuid``: Campaign UUID
* ``Name``: <optional> Update campaign name.
* ``Detail``: <optional> Update campaign description.
* ``Status``: <optional> Update campaign status.
* ``Plan``: <optional> Update campaign plan.
* ``Dlma``: <optional> Update campaign dlma.

Returns
-------
::

    Response: Success
    Message: Campaign updated successfully


Example
-------
::

   Action: OutCampaignUpdate
   Uuid: 0198236d-0ffd-4c8d-b3fb-133d976f8a01
   Name: Test campaign 03
   
   Response: Success
   Message: Campaign updated successfully
   
   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: 0198236d-0ffd-4c8d-b3fb-133d976f8a01
   Name: Test campaign 03
   Detail: <unknown>
   Status: 0
   Plan: <unknown>
   Dlma: <unknown>
   TmCreate: 2016-10-03 20:05:49.429024
   TmDelete: <unknown>
   TmUpdate: 2016-10-03 20:07:05.932267

    
OutCampaignDelete
=================
Description
-----------
Delete exist campaign info.

Syntax
------

::

    Action: OutCampaignDelete
    ActionID: <value>
    Uuid: <value>

Parameters

* ``Uuid``: Campaign UUID

Returns
-------
::

    Response: Success
    Message: Campaign deleted successfully


Example
-------
::

   Action: OutCampaignDelete
   Uuid: 0198236d-0ffd-4c8d-b3fb-133d976f8a01
   
   Response: Success
   Message: Campaign deleted successfully
   
   Event: OutCampaignDelete
   Privilege: message,all
   Uuid: 0198236d-0ffd-4c8d-b3fb-133d976f8a01


OutCampaignShow
===============
Description
-----------
Show specified|all campaign info.

Syntax
------

::

    Action: OutCampaignShow
    ActionID: <value>
    [Uuid:] <value>

Parameters

* ``Uuid``: Campaign UUID

Returns
-------
::

    Response: Success
    EventList: start
    Message: Campaign List will follow

    ...
    
    Event: OutCampaignListComplete
    EventList: Complete
    ListItems: 1

Example
-------
::

    Action: OutCampaignShow

    Response: Success
    EventList: start
    Message: Campaign List will follow

    Event: OutCampaignEntry
    Uuid: c82831f1-b1c2-46ca-86f9-3bd41f45773c
    Name: test campaign 02
    Detail: The test campaign 02
    Status: 0
    Plan: <unknown>
    Dlma: <unknown>
    TmCreate: 2016-10-02 14:43:33.858693
    TmDelete: <unknown>
    TmUpdate: <unknown>

    Event: OutCampaignListComplete
    EventList: Complete
    ListItems: 1

OutPlanCreate
=============
Description
-----------
Create a new plan.

Syntax
------

::

    Action: OutPlanCreate
    ActionID: <value>
    [Name:] <value>
    [Detail:] <value>
    [DialMode:] <value>
    [CallerId:] <value>
    [AnswerHandle:] <value>
    [DlEndHandle:] <value>
    [RetryDelay:] <value>
    [TrunkName:] <value>
    [QueueName:] <value>
    [AmdMode:] <value>
    [MaxRetry1:] <value>
    [MaxRetry2:] <value>
    [MaxRetry3:] <value>
    [MaxRetry4:] <value>
    [MaxRetry5:] <value>
    [MaxRetry6:] <value>
    [MaxRetry7:] <value>
    [MaxRetry8:] <value>


Parameters


Returns
-------
::
   
   Response: Success
   Message: Plan created successfully

Example
-------
::

   Action: OutPlanCreate

   Response: Success
   Message: Plan created successfully
   
   Event: OutPlanCreate
   Privilege: message,all
   Uuid: b9a6f7b6-e3ea-4e08-839c-c51e0ad196d6
   Name: <unknown>
   Detail: <unknown>
   DialMode: 0
   DialTimeout: 30000
   CallerId: <unknown>
   AnswerHandle: 0
   DlEndHandle: 1
   RetryDelay: 50000
   TrunkName: <unknown>
   QueueName: <unknown>
   AmdMode: 0
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-10-02 21:46:38.651717
   TmDelete: <unknown>
   TmUpdate: <unknown>


OutPlanUpdate
=============

Description
-----------
Update a exist plan info.

Syntax
------

::

    Action: OutPlanUpdate
    ActionID: <value>
    Uuid: <value>
    [Name:] <value>
    [Detail:] <value>
    [DialMode:] <value>
    [CallerId:] <value>
    [AnswerHandle:] <value>
    [DlEndHandle:] <value>
    [RetryDelay:] <value>
    [TrunkName:] <value>
    [QueueName:] <value>
    [AmdMode:] <value>
    [MaxRetry1:] <value>
    [MaxRetry2:] <value>
    [MaxRetry3:] <value>
    [MaxRetry4:] <value>
    [MaxRetry5:] <value>
    [MaxRetry6:] <value>
    [MaxRetry7:] <value>
    [MaxRetry8:] <value>



Parameters


Returns
-------
::
    

Example
-------
::

    

OutPlanDelete
=============

Description
-----------
Delete a exist plan info.

Syntax
------

::

    Action: OutPlanDelete
    ActionID: <value>
    Uuid: <value>


Parameters


Returns
-------
::
    
   Response: Success
   ActionID: 5bda9fb8-88ec-11e6-a1a5-d719861709b2
   Message: Plan deleted successfully

Example
-------
::

   Action: OutPlanDelete
   ActionID: 5bda9fb8-88ec-11e6-a1a5-d719861709b2
   Uuid: fca7a70d-fefe-4264-b967-76e7784b0d92
   
   Response: Success
   ActionID: 5bda9fb8-88ec-11e6-a1a5-d719861709b2
   Message: Plan deleted successfully
   
   Event: OutPlanDelete
   Privilege: message,all
   Uuid: fca7a70d-fefe-4264-b967-76e7784b0d92
   

OutPlanShow
===========

Description
-----------
Show specified|all plan info

Syntax
------

::

    Action: OutPlanShow
    ActionID: <value>
    [Uuid:] <value>


Parameters


Returns
-------
::

   Response: Success
   EventList: start
   Message: Plan List will follow
   
   ...
   
   Event: OutPlanListComplete
   EventList: Complete
   ListItems: 31
   

Example
-------
::

   Action: OutPlanShow

   Response: Success
   EventList: start
   Message: Plan List will follow
   
   Event: OutPlanEntry
   Uuid: 015280bf-8d46-4e42-8f16-72a22cda42d3
   Name: <unknown>
   Detail: <unknown>
   DialMode: 0
   DialTimeout: 30000
   CallerId: <unknown>
   AnswerHandle: 0
   DlEndHandle: 1
   RetryDelay: 50000
   TrunkName: <unknown>
   QueueName: <unknown>
   AmdMode: 0
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-10-02 20:19:08.478190
   TmDelete: <unknown>
   TmUpdate: <unknown>
   
   ...
   
   Event: OutPlanListComplete
   EventList: Complete
   ListItems: 31
   
   


OutDlmaCreate
=============

Description
-----------
Create new dlma.

Syntax
------

::

    Action: OutDlmaCreate
    ActionID: <value>
    [Name:] <value>
    [Detail:] <value>


Parameters


Returns
-------
::
        
    Response: Success
    Message: Dlma created successfully

Example
-------
::

    Action: OutDlmaCreate

    Response: Success
    Message: Dlma created successfully

    Event: OutDlmaCreate
    Privilege: message,all
    Uuid: 0853bbaa-7366-4c46-9320-fe5daf92a56b
    Name: <unknown>
    Detail: <unknown>
    DlTable: 0853bbaa_7366_4c46_9320_fe5daf92a56b
    TmCreate: 2016-10-02 15:40:14.939472
    TmDelete: <unknown>
    TmUpdate: <unknown>

    
OutDlmaUpdate
=============

Description
-----------
Update exist dlma info.

Syntax
------

::

    Action: OutDlmaUpdate
    ActionID: <value>
    Uuid: <value>
    [Name:] <value>
    [Detail:] <value>


Parameters


Returns
-------
::
        
    Response: Success
    Message: Dlma updated successfully

Example
-------
::

    Action: OutDlmaUpdate
    Uuid: 0853bbaa-7366-4c46-9320-fe5daf92a56b
    Name: Test dlma info
    Detail: test dlma

    Response: Success
    Message: Dlma updated successfully

    Event: OutDlmaUpdate
    Privilege: message,all
    Uuid: 0853bbaa-7366-4c46-9320-fe5daf92a56b
    Name: Test dlma info
    Detail: test dlma
    DlTable: 0853bbaa_7366_4c46_9320_fe5daf92a56b
    TmCreate: 2016-10-02 15:40:14.939472
    TmDelete: <unknown>
    TmUpdate: 2016-10-02 15:42:36.595071

    
OutDlmaDelete
=============

Description
-----------
Delete exist dlma info.

Syntax
------

::

    Action: OutDlmaDelete
    ActionID: <value>
    Uuid: <value>


Parameters


Returns
-------
::
        
    Response: Success
    Message: Dlma deleted successfully

Example
-------
::

    Action: OutDlmaDelete
    Uuid: 0853bbaa-7366-4c46-9320-fe5daf92a56b

    Response: Success
    Message: Dlma deleted successfully

    Event: OutDlmaDelete
    Privilege: message,all
    Uuid: 0853bbaa-7366-4c46-9320-fe5daf92a56b


OutDlmaShow
===========

Description
-----------
Show all|specified exist dlma info.

Syntax
------

::

    Action: OutDlmaShow
    ActionID: <value>
    [Uuid:] <value>


Parameters


Returns
-------
::
        
    Response: Success
    EventList: start
    Message: Dlma List will follow

    ...
    
    Event: OutDlmaListComplete
    EventList: Complete
    ListItems: 1

Example
-------
::

    Action: OutDlmaShow

    Response: Success
    EventList: start
    Message: Dlma List will follow

    Event: OutDlmaEntry
    Uuid: 0853bbaa-7366-4c46-9320-fe5daf92a56b
    Name: Test dlma info
    Detail: test dlma
    DlTable: 0853bbaa_7366_4c46_9320_fe5daf92a56b
    TmCreate: 2016-10-02 15:40:14.939472
    TmDelete: <unknown>
    TmUpdate: 2016-10-02 15:42:36.595071

    Event: OutDlmaListComplete
    EventList: Complete
    ListItems: 1

    
OutDlListCreate
===============

Description
-----------
Create Dial list for dialing.

Syntax
------

::

   Action: OutDlListCreate
   ActionID: <value>
   DlmaUuid: <dlma-uuid>
   Name: <customer-name>
   Detail: <customer-detail info>
   UKey: <customer-unique key>
   UData: <customer-UUI data>
   Number1: <customer-destination 1>
   Number2: <customer-destination 2>
   Number3: <customer-destination 3>
   Number4: <customer-destination 4>
   Number5: <customer-destination 5>
   Number6: <customer-destination 6>
   Number7: <customer-destination 7>
   Number8: <customer-destination 8>
   res_dial: <dial-result>
   res_dial_detail: <dial-result-detail>
   res_hangup: <dial-hangup>
   res_hangup_detail: <dial-hangup-detail>


Parameters


Returns
-------
::
        
    Response: Success
    EventList: start
    Message: Dlma List will follow

    ...
    
    Event: OutDlmaListComplete
    EventList: Complete
    ListItems: 1

Example
-------
::

   Action: OutDlListCreate
   DlmaUuid: 6c1e916a-608e-494c-9350-5a7095d6f640
   Name: client 01
   Detail: Dial to client 01
   Number1: sip:client-01@example.com
   
   Response: Success
   Message: Dl list created successfully
   
