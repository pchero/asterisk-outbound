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
    Uuid: 5686822c-f596-483c-acaa-e96c12294e68
    Name: <unknown>
    Detail: <unknown>
    Status: 0
    Plan: (null)
    Dlma: <unknown>
    TmCreate: <unknown>
    TmDelete: 2016-10-02 14:28:45.812672
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
    ActionID: 4eebd84c-fff8-4562-9d61-7889dcb57ab4
    Uuid: 5686822c-f596-483c-acaa-e96c12294e68
    Name: test campaign 01
    Detail: The test campaign 01

    Response: Success
    ActionID: 4eebd84c-fff8-4562-9d61-7889dcb57ab4
    Message: Campaign updated successfully

    Event: OutCampaignUpdate
    Privilege: message,all
    Uuid: 5686822c-f596-483c-acaa-e96c12294e68
    Name: test campaign 01
    Detail: The test campaign 01
    Status: 0
    Plan: (null)
    Dlma: <unknown>
    TmCreate: <unknown>
    TmDelete: 2016-10-02 14:28:45.812672
    TmUpdate: <unknown>

    
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
    ActionID: 3c827c64-88ae-11e6-b5fd-938a04bec737
    Uuid: 5686822c-f596-483c-acaa-e96c12294e68

    Response: Success
    ActionID: 3c827c64-88ae-11e6-b5fd-938a04bec737
    Message: Campaign deleted successfully

    Event: OutCampaignDelete
    Privilege: message,all
    Uuid: 5686822c-f596-483c-acaa-e96c12294e68


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
    Plan: (null)
    Dlma: <unknown>
    TmCreate: <unknown>
    TmDelete: 2016-10-02 14:43:33.858693
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
    

Example
-------
::


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
