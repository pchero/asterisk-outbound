.. ami_event

*********
AMI Event
*********

OutCampaignCreate
=================

Description
-----------
Notify event message for campaign has been created.

Syntax
------

::

   Event: OutCampaignCreate
   Uuid: <value>
   Name: <value>
   Detail: <value>
   Status: <value>
   Plan: <value>
   Dlma: <value>
   Dest: <value>
   ScMode: <value>
   ScTimeStart: <value>
   ScTimeEnd: <value>
   ScDateStart: <value>
   ScDateEnd: <value>
   ScDateList: <value>
   ScDateListExcept: <value>
   ScDayList: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Uuid : Campaign uuid.
* Name : Campaign name.
* Detail : Campaign detail info.
* Status : Campaign status. See detail :ref:`campaign_status`.
* Plan : Registered plan uuid.
* Dlma : Registered dlma uuid.
* Dest : Registered destination uuid.
* ScMode : Scheduling mode. See detail :ref:`scheduling_mode`.
* ScTimeStart : Campaign scheduling start time. See detail :ref:`scheduling_time`.
* ScTimeEnd : Campaign scheduling end time. See detail :ref:`scheduling_time`.
* ScDateStart : Campaign scheduling start date. See detail :ref:`scheduling_date`.
* ScDateEnd : Campaign scheduling end date. See detail :ref:`scheduling_date`.
* ScDateList : Campaign schedling date list. See detail :ref:`scheduling_date_list`.
* ScDateListExcept : Campaign scheduling except date list. See detail :ref:`scheduling_date_list`.
* ScDayList : Campaign scheduling day list. See detail :ref:`scheduling_day_list`.

Example
-------

::

   Event: OutCampaignCreate
   Privilege: message,all
   Uuid: 9b098077-e265-4653-8e7c-838ff4dcd268
   Name: <unknown>
   Detail: <unknown>
   Status: 0
   Plan: <unknown>
   Dlma: <unknown>
   Dest: <unknown>
   ScMode: 0
   ScTimeStart: <unknown>
   ScTimeEnd: <unknown>
   ScDateStart: <unknown>
   ScDateEnd: <unknown>
   ScDateList: <unknown>
   ScDateListExcept: <unknown>
   ScDayList: <unknown>
   TmCreate: 2016-11-14T23:01:00.489425780Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


OutCampaignUpdate
=================

Description
-----------
Notify event message for updated campaign info.

Syntax
------

::

   Event: OutCampaignUpdate
   Uuid: <value>
   Name: <value>
   Detail: <value>
   Status: <value>
   Plan: <value>
   Dlma: <value>
   Dest: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Uuid: Campaign uuid.
* Name: Campaign name.
* Detail: Campaign detail info.
* Status: Campaign status. See detail :ref:`campaign_status`.
* Plan: Registered plan uuid.
* Dlma: Registered dlma uuid.
* Dest: Registered destination uuid.

Example
-------

::

   Event: OutCampaignUpdate
   Privilege: message,all
   Uuid: a3e3cc0e-083e-4b9e-8120-522eb9834098
   Name: test
   Detail: <unknown>
   Status: 0
   Plan: <unknown>
   Dlma: <unknown>
   Dest: <unknown>
   TmCreate: 2016-10-23T23:59:18.883765584Z
   TmDelete: <unknown>
   TmUpdate: 2016-10-24T00:01:29.706507679Z


OutCampaignDelete
=================

Description
-----------
Notify event message for deleted campaign info.

Syntax
------

::

   Event: OutCampaignDelete
   Privilege: message,all
   Uuid: <value>

Parameters

* Uuid: Campaign uuid.

Example
-------

::

   Event: OutCampaignDelete
   Privilege: message,all
   Uuid: 92dadd08-ac3c-47dc-a41b-10040643ee38


OutPlanCreate
=============

Description
-----------
Notify event message for created plan info.

Syntax
------

::

   Event: OutPlanCreate
   Uuid: <value>
   Name: <value>
   Detail: <value>
   DialMode: <value>
   DialTimeout: <value>
   CallerId: <value>
   DlEndHandle: <value>
   RetryDelay: <value>
   TrunkName: <value>
   TechName: <value>
   MaxRetryCnt1: <value>
   MaxRetryCnt2: <value>
   MaxRetryCnt3: <value>
   MaxRetryCnt4: <value>
   MaxRetryCnt5: <value>
   MaxRetryCnt6: <value>
   MaxRetryCnt7: <value>
   MaxRetryCnt8: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Name: Plan name.
* Detail: Detail info.
* DialMode: Dialling mode. See detail :ref:`dial_mode`.
* UuiField: Customer's Uui field name.
* DialTimeout: Ringing timeout(ms).
* CallerId: Caller's id.
* DlEndHandle: Determine behavior of when the dial list end. See detail :ref:`dial_list_end_handling`.
* RetryDelay: Delay time for next try(sec).
* TrunkName: Trunkname for outbound dialing.
* TechName: Tech name for outbound dialing. See detail :ref:`tech_name`.
* ServiceLevel: Determine service level.
* MaxRetry1: Max retry count for number 1.
* MaxRetry2: Max retry count for number 2.
* MaxRetry3: Max retry count for number 3.
* MaxRetry4: Max retry count for number 4.
* MaxRetry5: Max retry count for number 5.
* MaxRetry6: Max retry count for number 6.
* MaxRetry7: Max retry count for number 7. 
* MaxRetry8: Max retry count for number 8.

Example
-------

::

   Event: OutPlanCreate
   Privilege: message,all
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

OutPlanUpdate
=============

Description
-----------
Notify event message for plan updated.

Syntax
------

::

   Event: OutPlanUpdate
   Uuid: <value>
   Name: <value>
   Detail: <value>
   DialMode: <value>
   DialTimeout: <value>
   CallerId: <value>
   DlEndHandle: <value>
   RetryDelay: <value>
   TrunkName: <value>
   TechName: <value>
   MaxRetryCnt1: <value>
   MaxRetryCnt2: <value>
   MaxRetryCnt3: <value>
   MaxRetryCnt4: <value>
   MaxRetryCnt5: <value>
   MaxRetryCnt6: <value>
   MaxRetryCnt7: <value>
   MaxRetryCnt8: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Name: Plan name.
* Detail: Detail info.
* DialMode: Dialling mode. See detail :ref:`dial_mode`.
* UuiField: Customer's Uui field name.
* DialTimeout: Ringing timeout(ms).
* CallerId: Caller's id.
* DlEndHandle: Determine behavior of when the dial list end. See detail :ref:`dial_list_end_handling`.
* RetryDelay: Delay time for next try(sec).
* TrunkName: Trunkname for outbound dialing.
* TechName: Tech name for outbound dialing. See detail :ref:`tech_name`.
* ServiceLevel: Determine service level.
* MaxRetry1: Max retry count for number 1.
* MaxRetry2: Max retry count for number 2.
* MaxRetry3: Max retry count for number 3.
* MaxRetry4: Max retry count for number 4.
* MaxRetry5: Max retry count for number 5.
* MaxRetry6: Max retry count for number 6.
* MaxRetry7: Max retry count for number 7. 
* MaxRetry8: Max retry count for number 8.

Example
-------

::

   Event: OutPlanUpdate
   Privilege: message,all
   Uuid: 4ea35c4b-c2db-4a22-baef-443b5fadd677
   Name: sales_plan
   Detail: Plan update test
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
   TmUpdate: 2016-10-23T23:12:20.503366656Z


OutPlanDelete
=============

Description
-----------
Notify event message for deleted plan info.

Syntax
------

::

   Event: OutPlanDelete
   Uuid: <value>

Parameters

* Uuid: Plan uuid.

Example
-------

::

   Event: OutPlanDelete
   Privilege: message,all
   Uuid: 945e7631-047f-49a8-a389-fb52ebc8ca45

OutPlanEntry
============

Description
-----------
Plan entry.

Syntax
------

::

   Event: OutPlanEntry
   Uuid: <value>
   Name: <value>
   Detail: <value>
   DialMode: <value>
   DialTimeout: <value>
   CallerId: <value>
   DlEndHandle: <value>
   RetryDelay: <value>
   TrunkName: <value>
   TechName: <value>
   Variable: <value>
   MaxRetryCnt1: <value>
   MaxRetryCnt2: <value>
   MaxRetryCnt3: <value>
   MaxRetryCnt4: <value>
   MaxRetryCnt5: <value>
   MaxRetryCnt6: <value>
   MaxRetryCnt7: <value>
   MaxRetryCnt8: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Name: Plan name.
* Detail: Detail info.
* DialMode: Dialling mode. See detail :ref:`dial_mode`.
* UuiField: Customer's Uui field name.
* DialTimeout: Ringing timeout(ms).
* CallerId: Caller's id.
* DlEndHandle: Determine behavior of when the dial list end. See detail :ref:`dial_list_end_handling`.
* RetryDelay: Delay time for next try(sec).
* TrunkName: Trunkname for outbound dialing.
* TechName: Tech name for outbound dialing. See detail :ref:`tech_name`.
* ServiceLevel: Determine service level.
* MaxRetry1: Max retry count for number 1.
* MaxRetry2: Max retry count for number 2.
* MaxRetry3: Max retry count for number 3.
* MaxRetry4: Max retry count for number 4.
* MaxRetry5: Max retry count for number 5.
* MaxRetry6: Max retry count for number 6.
* MaxRetry7: Max retry count for number 7. 
* MaxRetry8: Max retry count for number 8.

Example
-------

::

   Event: OutPlanEntry
   Uuid: edbb8fb0-4543-4590-9d12-7ab2c66de3ea
   Name: <unknown>
   Detail: <unknown>
   DialMode: 1
   DialTimeout: 30000
   CallerId: <unknown>
   DlEndHandle: 1
   RetryDelay: 60
   TrunkName: <unknown>
   TechName: <unknown>
   Variable: <unknown>
   MaxRetryCnt1: 5
   MaxRetryCnt2: 5
   MaxRetryCnt3: 5
   MaxRetryCnt4: 5
   MaxRetryCnt5: 5
   MaxRetryCnt6: 5
   MaxRetryCnt7: 5
   MaxRetryCnt8: 5
   TmCreate: 2016-12-25T14:55:20.84455813Z
   TmDelete: <unknown>
   TmUpdate: <unknown>

OutDlmaCreate
=============

Description
-----------
Notify event message for dlma created.

Syntax
------

::

   Event: OutDlmaCreate
   Uuid: <value>
   Name: <value>
   Detail: <value>
   DlTable: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Uuid: Dlma uuid.
* Name: Dlma name.
* Detail: Dlma detail info.
* DlTable: Dlma reference table.

Example
-------

::

   Event: OutDlmaCreate
   Privilege: message,all
   Uuid: 9e2f750f-82e4-42c3-a06f-41b55056fdb0
   Name: <unknown>
   Detail: <unknown>
   DlTable: 9e2f750f_82e4_42c3_a06f_41b55056fdb0
   TmCreate: 2016-10-24T00:07:59.767803436Z
   TmDelete: <unknown>
   TmUpdate: <unknown>


OutDlmaUpdate
=============
Description
-----------
Notify event message for dlma updated.

Syntax
------

::

   Event: OutDlmaUpdate
   Uuid: <value>
   Name: <value>
   Detail: <value>
   DlTable: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>

Parameters

* Uuid: Dlma uuid.
* Name: Dlma name.
* Detail: Dlma detail info.
* DlTable: Dlma reference table.

Example
-------

::

   Event: OutDlmaUpdate
   Privilege: message,all
   Uuid: a0dc9df7-89bd-4c2c-ac81-bc7fbc54ff96
   Name: 8e6a4214-6e1c-47a3-946f-661e6cf58c04
   Detail: Change
   DlTable: a0dc9df789bd4c2cac81bc7fbc54ff96
   TmCreate: 2015-12-09 19:12:51.753941
   TmDelete: <unknown>
   TmUpdate: 2015-12-09 19:12:51.884059


OutDlmaDelete
=============

Description
-----------
Notify event message for dlma deleted.

Syntax
------

::

   Event: OutDlmaDelete
   Uuid: <value>

Parameters

* Uuid: Dlma uuid.

Example
-------

::

   Event: OutDlmaDelete
   Privilege: message,all
   Uuid: a0dc9df7-89bd-4c2c-ac81-bc7fbc54ff96

OutDlmaEntry
============

Description
-----------
Dlma entry

Example
-------

::

   Event: OutDlmaEntry
   Uuid: 5d56b51d-dc4d-4ec9-9e82-88e8dc3737c1
   Name: DialListMaster_Sales
   Detail: Test Dlma description
   DlTable: 5d56b51d_dc4d_4ec9_9e82_88e8dc3737c1
   Variable: <unknown>
   TmCreate: 2016-11-15T02:11:22.128567345Z
   TmDelete: <unknown>
   TmUpdate: <unknown>



OutDestinationCreate
====================

Example
-------

::

   Event: OutDestinationCreate
   Privilege: message,all
   Uuid: 3ff22c32-a727-4d0b-ba85-aa8aef58ddc0
   Name: test destination.
   Detail: detail test destination
   Type: 0
   Exten: <unknown>
   Context: <unknown>
   Priority: <unknown>
   Variable: test=good
   Variable: test1=good1
   Application: <unknown>
   Data: <unknown>
   TmCreate: 2016-10-24T00:41:39.178684973Z
   TmDelete: <unknown>
   TmUpdate: <unknown>

OutDestinationUpdate
====================

Example
-------

::

   Event: OutDestinationUpdate
   Privilege: message,all
   Uuid: 36612bfb-3830-4c77-b0f0-e74bb77fb3ac
   Name: update test destination
   Detail: detail test destination
   Type: 0
   Exten: <unknown>
   Context: <unknown>
   Priority: <unknown>
   Variable: <unknown>
   Application: <unknown>
   Data: <unknown>
   TmCreate: 2016-10-24T00:33:59.864623354Z
   TmDelete: <unknown>
   TmUpdate: 2016-10-24T00:47:11.304260041Z


OutDestinationDelete
====================

Example
-------

::

   Event: OutDestinationDelete
   Privilege: message,all
   Uuid: 36612bfb-3830-4c77-b0f0-e74bb77fb3ac
   
OutDestinationEntry
===================
Description
-----------
Destination entry.

Syntax
------

::

   Event: OutDestinationEntry
   Uuid: <value>
   Name: <value>
   Detail: <value>
   Type: <value>
   Exten: <value>
   Context: <value>
   Priority: <unknown>
   Variable: <unknown>
   Application: <value>
   Data: <value>
   TmCreate: <value>
   TmDelete: <value>
   TmUpdate: <value>
   
Parameters

* Uuid: Destination uuid.
* Name: Destination name.
* Detail: Destination detail info.
* Type: Destination type. See detail :ref:`destination_type`.
* Exten: Extension. Type: 0(exten) only
* Context: Conetxt. Type: 0(exten) only
* Priority: Priority. Type: 0(exten) only
* Variable: Set(var=val). Could be more than one. Type: 0(exten) only.
* Application: Application name. Type: 1(application) only
* Data: Application data. Type: 1(application) only
* TmCreate: <value>
* TmDelete: <value>
* TmUpdate: <value>

Example
-------

::

   Event: OutDestinationEntry
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


OutDialingCreate
================
Description
-----------
Notify event message for dialing created.

Syntax
------

Parameters

Example
-------

::

   Event: OutDialingCreate
   Privilege: message,all
   Uuid: 28863ee7-3e86-47cd-b87e-0894d328644c
   Status: 0
   CampUuid: 02c4aebf-789c-46aa-817e-b7406416d211
   PlanUuid: 4ea35c4b-c2db-4a22-baef-443b5fadd677
   DlmaUuid: acc994d2-04d9-4a53-bfcf-50c96ff924bc
   DlListUuid: dc5bf351-a63c-4dda-8a1f-2bf337ce4e45
   CurrentQueue: <unknown>
   CurrentAgent: <unknown>
   DialIndex: 1
   DialAddr: 300
   DialChannel: sip/300
   DialTryCnt: 1
   DialTimeout: 0
   DialType: 1
   DialExten: <unknown>
   DialContext: <unknown>
   DialApplication: park
   DialData: 
   ChannelName: <unknown>
   ResDial: 0
   ResAmd: <unknown>
   ResAmdDetail: <unknown>
   ResHangup: 0
   ResHangupDetail: <unknown>
   TmCreate: 2016-10-24T00:10:46.302114915Z
   TmUpdate: <unknown>
   TmDelete: <unknown>



OutDialingUpdate
================

Description
-----------
Notify event message for dialing updated.

Syntax
------

Parameters

Example
-------

::

   Event: OutDialingUpdate
   Privilege: message,all
   Uuid: 28863ee7-3e86-47cd-b87e-0894d328644c
   Status: 1
   CampUuid: 02c4aebf-789c-46aa-817e-b7406416d211
   PlanUuid: 4ea35c4b-c2db-4a22-baef-443b5fadd677
   DlmaUuid: acc994d2-04d9-4a53-bfcf-50c96ff924bc
   DlListUuid: dc5bf351-a63c-4dda-8a1f-2bf337ce4e45
   CurrentQueue: <unknown>
   CurrentAgent: <unknown>
   DialIndex: 1
   DialAddr: 300
   DialChannel: sip/300
   DialTryCnt: 1
   DialTimeout: 0
   DialType: 1
   DialExten: <unknown>
   DialContext: <unknown>
   DialApplication: park
   DialData: 
   ChannelName: SIP/300-00000000
   ResDial: 0
   ResAmd: <unknown>
   ResAmdDetail: <unknown>
   ResHangup: 0
   ResHangupDetail: <unknown>
   TmCreate: 2016-10-24T00:10:46.302114915Z
   TmUpdate: 2016-10-24T00:10:46.310483656Z
   TmDelete: <unknown>


OutDialingDelete
================

Description
-----------
Notify message for dialing deleted.

Syntax
------

::

   Event: OutDialingDelete
   Uuid: <value>


Parameters

* Uuid: Dialing uuid.

Example
-------

::

   Event: OutDialingDelete
   Privilege: message,all
   Uuid: 0db3d746-5185-42b9-9c5e-6bc95617ee00


