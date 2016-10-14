.. ami_event

*********
AMI Event
*********

Overview
========
AMI event messages from Asterisk-outbound


OutCampaignCreate
=================

Description
-----------
Notify event message for campaign has been created.

Syntax
------

::

  Event: OutCampaignCreate
  Privilege: message,all
  Uuid: <value>
  [Name:] <value>
  [Detail:] <value>
  Status: <value>
  Plan: <value>
  Dlma: <value>
  TmCreate: <timestamp>
  TmDelete: <timestamp>
  TmUpdate: <timestamp>
  OutCampaignUpdate

Parameters

* ``Uuid``: Campaign uuid.
* ``Name``: Campaign name.
* ``Detail``: Campaign detail info.
* ``Status``: Campaign status.
* ``Plan``: Registered plan uuid.
* ``Dlma``: Registered dlma uuid.

Example
-------

::

  Event: OutCampaignCreate
  Privilege: message,all
  Uuid: 92dadd08-ac3c-47dc-a41b-10040643ee38
  Name: 1009c510-71aa-4736-89f9-9b39473271cc
  Detail: TestDetail
  Status: 0
  Plan: 5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56
  Dlma: e276d8be-a558-4546-948a-f99913a7fea2
  TmCreate: 2015-12-09 13:29:39.483175
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
  Privilege: message,all
  Uuid: <value>
  [Name:] <value>
  [Detail:] <value>
  Status: <value>
  Plan: <value>
  Dlma: <value>
  TmCreate: <timestamp>
  TmDelete: <timestamp>
  TmUpdate: <timestamp>

Parameters

* ``Uuid``: Campaign uuid.
* ``Name``: Campaign name.
* ``Detail``: Campaign detail info.
* ``Status``: Campaign status.
* ``Plan``: Registered plan uuid.
* ``Dlma``: Registered dlma uuid.

Example
-------

::

  Event: OutCampaignUpdate
  Privilege: message,all
  Uuid: 92dadd08-ac3c-47dc-a41b-10040643ee38
  Name: 1009c510-71aa-4736-89f9-9b39473271cc
  Detail: Change
  Status: 0
  Plan: 5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56
  Dlma: e276d8be-a558-4546-948a-f99913a7fea2
  TmCreate: 2015-12-09 13:29:39.483175
  TmDelete: <unknown>
  TmUpdate: 2015-12-09 13:29:39.630756


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

* ``Uuid``: Campaign uuid.

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

Parameters

Example
-------

::

  Event: OutPlanCreate
  Privilege: message,all
  Uuid: 945e7631-047f-49a8-a389-fb52ebc8ca45
  Name: TestPlan
  Detail: TestDetail
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
  TmCreate: 2015-12-09 13:58:33.765672
  TmDelete: <unknown>
  TmUpdate: <unknown>


OutPlanUpdate
=============

Description
-----------
Notify event message for plan updated.

Syntax
------

Parameters

Example
-------

::

  Event: OutPlanUpdate
  Privilege: message,all
  Uuid: 945e7631-047f-49a8-a389-fb52ebc8ca45
  Name: TestPlan
  Detail: Change
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
  TmCreate: 2015-12-09 13:58:33.765672
  TmDelete: <unknown>
  TmUpdate: 2015-12-09 13:58:33.945415


OutPlanDelete
=============

Description
-----------
Notify event message for deleted plan info.

Syntax
------

Parameters
----------

Example
-------

::

  Event: OutPlanDelete
  Privilege: message,all
  Uuid: 945e7631-047f-49a8-a389-fb52ebc8ca45


OutDlmaCreate
=============

Description
-----------
Notify event message for dlma created.

Syntax
-------

Parameters

Example
-------

::

  Event: OutDlmaCreate
  Privilege: message,all
  Uuid: a0dc9df7-89bd-4c2c-ac81-bc7fbc54ff96
  Name: 8e6a4214-6e1c-47a3-946f-661e6cf58c04
  Detail: TestDetail
  DlTable: a0dc9df789bd4c2cac81bc7fbc54ff96
  TmCreate: 2015-12-09 19:12:51.753941
  TmDelete: <unknown>
  TmUpdate: <unknown>


OutDlmaUpdate
=============
Description
-----------
Notify event message for dlma updated.

Syntax
------

Parameters

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

Parameters

Example
-------

::

  Event: OutDlmaDelete
  Privilege: message,all
  Uuid: a0dc9df7-89bd-4c2c-ac81-bc7fbc54ff96


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
  Uuid: 14e8e861-232f-49fb-8101-f5ae489d94d6
  Status: 0
  CampUuid: 8cd1d05b-ad45-434f-9fde-4de801dee1c7
  PlanUuid: 5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56
  DlmaUuid: e276d8be-a558-4546-948a-f99913a7fea2
  DlListUuid: 382b639c-9eba-11e5-a926-0800271f0a4b
  CurrentQueue: <null>
  CurrentAgent: <null>
  DialIndex: 1
  DialAddr: 111-111-0001
  DialChannel: SIP/111-111-0001@trunk_test_1
  DialTryCnt: 1
  DialTimeout: 30000
  DialType: 0
  DialExten: 5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56
  DialContext: res_outbound
  DialApplication: <unknown>
  DialData: <unknown>
  ChannelName: <unknown>
  ResDial: 0
  ResAmd: <unknown>
  ResAmdDetail: <unknown>
  ResHangup: 0
  ResHangupDetail: <unknown>
  TmCreate: 2015-12-09T21:17:22.209160178Z
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
  Uuid: 14e8e861-232f-49fb-8101-f5ae489d94d6
  Status: 1
  CampUuid: 8cd1d05b-ad45-434f-9fde-4de801dee1c7
  PlanUuid: 5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56
  DlmaUuid: e276d8be-a558-4546-948a-f99913a7fea2
  DlListUuid: 382b639c-9eba-11e5-a926-0800271f0a4b
  CurrentQueue: <null>
  CurrentAgent: <null>
  DialIndex: 1
  DialAddr: 111-111-0001
  DialChannel: SIP/111-111-0001@trunk_test_1
  DialTryCnt: 1
  DialTimeout: 30000
  DialType: 0
  DialExten: 5ad6c7d8-535c-4cd3-b3e5-83ab420dcb56
  DialContext: res_outbound
  DialApplication: <unknown>
  DialData: <unknown>
  ChannelName: SIP/trunk_test_1-00000007
  ResDial: 0
  ResAmd: <unknown>
  ResAmdDetail: <unknown>
  ResHangup: 0
  ResHangupDetail: <unknown>
  TmCreate: 2015-12-09T21:17:22.209160178Z
  TmUpdate: 2015-12-09T21:17:22.249904462Z
  TmDelete: <unknown>


OutDialingDelete
================

Description
-----------
Notify message for dialing deleted.

Syntax
------

Parameters

Example
-------

::

  Event: OutDialingDelete
  Privilege: message,all
  Uuid: 4100781e-88f6-403d-af46-9500335d5560


