.. ami_action

**********
AMI Action
**********

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
    [Dest:] <value>

Parameters

* ``Name``: Name of campaign.
* ``Detail``: Description of campaign.
* ``Plan``: Plan uuid.
* ``Dlma``: Dlma uuid.
* ``Dest``: Destination uuid.

Returns
-------
::

    Response: Success
    Message: Campaign created successfully

Example
-------
::

   Action: OutCampaignCreate
   Name: Sales campaign
   Detail: test campaign
   Plan: 4ea35c4b-c2db-4a22-baef-443b5fadd677
   Dlma: acc994d2-04d9-4a53-bfcf-50c96ff924bc
   Dest: 4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94
   
   Response: Success
   Message: Campaign created successfully


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
    Uuid: 02c4aebf-789c-46aa-817e-b7406416d211

    Response: Success
    EventList: start
    Message: Campaign List will follow

    Event: OutCampaignEntry
    Uuid: 02c4aebf-789c-46aa-817e-b7406416d211
    Name: Sales campaign
    Detail: test campaign
    Status: 0
    Plan: 4ea35c4b-c2db-4a22-baef-443b5fadd677
    Dlma: acc994d2-04d9-4a53-bfcf-50c96ff924bc
    Dest: 4e6ed9e6-5dd2-409a-b6fe-a07ca11b1e94
    TmCreate: 2016-10-22T14:34:45.33929956Z
    TmDelete: <unknown>
    TmUpdate: 2016-10-22T15:30:55.226737231Z

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

   [Name:] <value>
   [Detail:] <value>
   [DialMode:] <value>
   [UuiField:] <value>
   [DialTimeout:] <value>
   [CallerId:] <value>
   [DlEndHandle:] <value>
   [RetryDelay:] <value>
   [TrunkName:] <value>
   [TechName:] <value>
   [ServiceLevel:] <value>
   [MaxRetry1:] <value>
   [MaxRetry2:] <value>
   [MaxRetry3:] <value>
   [MaxRetry4:] <value>
   [MaxRetry5:] <value>
   [MaxRetry6:] <value>
   [MaxRetry7:] <value>
   [MaxRetry8:] <value>


Parameters
----------

* Name: Plan name. Default null.
* Detail: Detail info. Default null.
* DialMode: Dialling mode. Default 1. See detail :ref:`dial_mode`.
* UuiField: Customer's Uui field name.
* DialTimeout: Ringing timeout(ms). Default 30000.
* CallerId: Caller's id. Default null.
* DlEndHandle: Determine behavior of when the dial list end. Default 1. See detail :ref:`dial_list_end_handling`.
* RetryDelay: Delay time for next try(sec). Default 60.
* TrunkName: Trunkname for outbound dialing. Default null.
* TechName: Tech name for outbound dialing. Default null. See detail :ref:`tech_name`.
* ServiceLevel: Determine service level. Default 0.
* MaxRetry1: Max retry count for number 1. Default 5
* MaxRetry2: Max retry count for number 2. Default 5
* MaxRetry3: Max retry count for number 3. Default 5
* MaxRetry4: Max retry count for number 4. Default 5
* MaxRetry5: Max retry count for number 5. Default 5
* MaxRetry6: Max retry count for number 6. Default 5
* MaxRetry7: Max retry count for number 7. Default 5
* MaxRetry8: Max retry count for number 8. Default 5


Returns
-------
::
   
   Response: Success
   Message: Plan created successfully

Example
-------
::

   Action: OutPlanCreate
   Name: sales_plan
   Detail: simple sales plan
   DialMode: 1
   QueueName: sales_1
   TechName: sip/
   
   Response: Success
   Message: Plan created successfully


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
    [UuiField:] <value>
    [DialTimeout:] <value>
    [CallerId:] <value>
    [DlEndHandle:] <value>
    [RetryDelay:] <value>
    [TrunkName:] <value>
    [TechName:] <value>
    [ServiceLevel:] <value>
    [MaxRetry1:] <value>
    [MaxRetry2:] <value>
    [MaxRetry3:] <value>
    [MaxRetry4:] <value>
    [MaxRetry5:] <value>
    [MaxRetry6:] <value>
    [MaxRetry7:] <value>
    [MaxRetry8:] <value>

Parameters
----------

* Name: Plan name. Default null.
* Detail: Detail info. Default null.
* DialMode: Dialling mode. Default 1. See detail :ref:`dial_mode`.
* UuiField: Customer's Uui field name.
* DialTimeout: Ringing timeout(ms). Default 30000.
* CallerId: Caller's id. Default null.
* DlEndHandle: Determine behavior of when the dial list end. Default 1. See detail :ref:`dial_list_end_handling`.
* RetryDelay: Delay time for next try(sec). Default 60.
* TrunkName: Trunkname for outbound dialing. Default null.
* TechName: Tech name for outbound dialing. Default null. See detail :ref:`tech_name`.
* ServiceLevel: Determine service level. Default 0.
* MaxRetry1: Max retry count for number 1. Default 5
* MaxRetry2: Max retry count for number 2. Default 5
* MaxRetry3: Max retry count for number 3. Default 5
* MaxRetry4: Max retry count for number 4. Default 5
* MaxRetry5: Max retry count for number 5. Default 5
* MaxRetry6: Max retry count for number 6. Default 5
* MaxRetry7: Max retry count for number 7. Default 5
* MaxRetry8: Max retry count for number 8. Default 5


Returns
-------
::

    Response: Success
    Message: Plan updated successfully
 

Example
-------
::

    Action: OutPlanUpdate
    Uuid: 4ea35c4b-c2db-4a22-baef-443b5fadd677
    Detail: Plan update test

    Response: Success
    Message: Plan updated successfully
 

OutPlanDelete
=============

Description
-----------
Delete a exist plan info.

Syntax
------

::

    Action: OutPlanDelete
    Uuid: <value>
    [ActionID:] <value>

Parameters
----------
* Uuid: Plan uuid.


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
Show specified|all plan info. 
If no uuid given, it shows all plans info.

Syntax
------

::

    Action: OutPlanShow
    [ActionID:] <value>
    [Uuid:] <value>


Parameters
----------

* Uuid: Plan uuid.


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

    Response: Success
    EventList: start
    Message: Plan List will follow

    Event: OutPlanEntry
    Uuid: 4ea35c4b-c2db-4a22-baef-443b5fadd677
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
    TmCreate: 2016-10-22T12:45:58.868877001Z
    TmDelete: <unknown>
    TmUpdate: <unknown>

    Event: OutPlanListComplete
    EventList: Complete
    ListItems: 1


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
----------
* Name: Dlma name.
* Detail: Detail dlma info.

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
----------
* Name: Dlma name.
* Detail: Detail dlma info.

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
----------
* Uuid: dlma uuid.

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
* Uuid: Dlma uuid.

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
   Message: Dl list created successfully

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
   

OutDestinationCreate
====================

Description
-----------
Create Destination for dialing.

Syntax
------

::

   Action: OutDestinationCreate
   [Name:] <value>
   [Detail:] <value>
   [Type:] <value>
   [Exten:] <value>
   [Context:] <value>
   [Priority:] <value>
   [Variable:] <var=value>
   [Application:] <value>
   [Data:] <value>

Parameters
* Name: Destination name.
* Detail: Detail info.
* Type: Destination type. See detail :ref:`destination_type`.
* Exten: Extension. Type: 0(exten) only
* Context: Conetxt. Type: 0(exten) only
* Priority: Priority. Type: 0(exten) only
* Variable: Set(var=val). Could be more than one. Type: 0(exten) only.
* Application: Application name. Type: 1(application) only
* Data: Application name. Type: 1(application) only

Example
-------

::

   Action: OutDestinationCreate
   Name: destination test
   Detail: test destination
   Type: 1
   Application: park
   Variable: var1=val1
   Variable: var2=val2
   
   Response: Success
   Message: Destination created successfully


OutDestinationUpdate
====================

Description
-----------
Update Destination for dialing.

Syntax
------

::

   Action: OutDestinationUpdate
   Uuid: <value>
   [Name:] <value>
   [Detail:] <value>
   [Type:] <value>
   [Exten:] <value>
   [Context:] <value>
   [Priority:] <value>
   [Variable:] <var=value>
   [Application:] <value>
   [Data:] <value>

Parameters
* Uuid: Destination uuid.
* Name: Destination name.
* Detail: Detail info.
* Type: Destination type. See detail :ref:`destination_type`.
* Exten: Extension. Type: 0(exten) only
* Context: Conetxt. Type: 0(exten) only
* Priority: Priority. Type: 0(exten) only
* Variable: Set(var=val). Could be more than one. Type: 0(exten) only.
* Application: Application name. Type: 1(application) only
* Data: Application name. Type: 1(application) only

Example
-------

::

   Action: OutDestinationUpdate
   Uuid: 36612bfb-3830-4c77-b0f0-e74bb77fb3ac
   Name: update test destination
   
   Response: Success
   Message: Destination updated successfully



OutDestinationDelete
====================

Description
-----------
Delete Destination for dialing.

Syntax
------

::

   Action: OutDestinationDelete
   Uuid: <value>

Parameters
* Uuid: <required> Destination uuid.
   
Example
-------

::

   Action: OutDestinationDelete
   Uuid: 36612bfb-3830-4c77-b0f0-e74bb77fb3ac
   
   Response: Success
   Message: Destination deleted successfully
