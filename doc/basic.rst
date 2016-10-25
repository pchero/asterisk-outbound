.. basic

*****
Basic
*****

Concept
=======

Principle
---------
The asterisk-outbound has a 5W1H principle.

.. figure:: _static/Basic_concept.png
   :align: center
   
   Basic concept

* Campaign : Determine why make a call(Why).
* Plan : Determine how/when to make a call(How/When).
* Destination : Determine who get a call after answer(Who).
* Dial list(Dial List Master) : Determine where/what call heading for(Where/What).

Resources
=========

Campaign
--------
* Set destination, plan, dial list(dlma).

Destination
-----------
* Call's transfer destination after customer has answered.

Plan
----
Dialling stragegy.
* UUI delivery.
* Dial mode
* Dialing control.

Dial list
---------
Customer info list.
* Dial number.
* UUI info.

Campaign
========
Determine why make a call(Why).

To dial to the customer, the user need to create the campaign first and need to assign the correct resources(destination, plan, dial list master).

Then the resources determine where/what/who/how/when make a call to the customer.

Plan
====
Determine how/when to make a call(How/When).

Dial mode
---------

Predictive
++++++++++
* Predict the number of customers to dial based on the deliver application/agent's answer rate.
* Predict how many call will be answered or not answered.
* Calculate possilbilties automatically.

Preview
+++++++
* The destination makes decision to make a call.
* Developing.

SMS
+++
* Send an SMS messages
* Developing.

Fax
+++
* Send a fax
* Developing.


Service level
-------------
Service level controling the amount of dailing.


.. _service_level:

::

   (Max available outbound call count) - (Current outbound call count) + (Service level) = (Available call count)

   If the (Available call count) is bigger than 0, make a call.


Max available outbound call count
++++++++++++++++++++++++++++++++++

The max available outbound call count is depends on the destination.

See detail :ref:`destination`.


.. _destination:

Destination
===========
Determine who get a call after answer(Who). And it determine the max available outbound call count.

Normaly, the destination suppose to be an agent. But in the asterisk system, the destination could be anything. For example, it could be extension or application(queue).

If the destination type is application, then the res_outbound calcaulate applciation's availablity.

Destination type
----------------

.. _destination_type:
.. table:: Destination type

   ==== ==================
   Type Detail
   ==== ==================
   0    Extensioin
   1    Application
   ==== ==================


Application availability
++++++++++++++++++++++++

.. _application_availability
.. table:: Application availability

   =========== =========================
   Application Detail
   =========== =========================
   queue       QueueSummary's Available.
   park        Unlimited.
   others      Unlimited.
   =========== =========================


Result
======
Result data devided by 5 sections.

Identity info, Dial info, Result info, Timestamp info, Related resources info.

::

   {
      "dialing_uuid": <value>,
      "camp_uuid": <value>,
      "plan_uuid": <value>,
      "dlma_uuid": <value>,
      "dest_uuid": <value>,
      "dl_list_uuid": <value>,
   
      "channel_name": <value>,   
      "dial_addr": <value>,
      "dial_trycnt": <value>,
      "dial_channel": <value>,
      "dial_index": <value>,
      "dial_data": <value>,
      "dial_type": <value>,
      "dial_application": <value>,
   
      "res_dial": <value>,
      "res_hangup": <value>,
      "res_hangup_detail": <value>,
   
      "tm_dial_begin": <value>,
      "tm_dial_end":   <value>,
      "tm_dialing":    <value>,
      "tm_hangup":     <value>,
   
      "info_camp": <value>,
      "info_plan": <value>,
      "info_dlma": <value>,
      "info_dest": <value>,
      "info_dial": <value>,
      "info_dl_list": <value>,
   
   }

Identity info
-------------
The identity info shows dialing's identity.

::

   "dialing_uuid": <value>,
   "camp_uuid": <value>,
   "plan_uuid": <value>,
   "dlma_uuid": <value>,
   "dest_uuid": <value>,
   "dl_list_uuid": <value>,

Dial info
---------

The dial info shows how to make a call.

::

   "channel_name": <value>,   
   "dial_addr": <value>,
   "dial_trycnt": <value>,
   "dial_channel": <value>,
   "dial_index": <value>,
   "dial_data": <value>,
   "dial_type": <value>,
   "dial_application": <value>,

Result info
-----------

The result info shows how the call end.

::

   "res_dial": <value>,
   "res_hangup": <value>,
   "res_hangup_detail": <value>,

Timestamp info
--------------

The timestamp info shows when the dail begun and ended.

::

   "tm_dial_begin": <value>,
   "tm_dial_end":   <value>,
   "tm_dialing":    <value>,
   "tm_hangup":     <value>,

Resource info
-------------

The resource info shows original resource info when the dial info created.

::

   "info_camp": <value>,
   "info_plan": <value>,
   "info_dlma": <value>,
   "info_dest": <value>,
   "info_dial": <value>,
   "info_dl_list": <value>,


Example
-------

::

   {
      "dialing_uuid": "a624ecec-e3a8-4e95-9538-abed6e2271ab",
      "camp_uuid": "ea289ed8-92f3-430c-b00c-b5254257282b",
      "plan_uuid": "5acea376-195a-4519-b68f-58e9ceaadc68",
      "dlma_uuid": "8f1cda4d-1a95-4cbc-9865-fb604ce3f70a",
      "dl_list_uuid": "8e0d1ef2-faf0-42d8-a70a-b494cae7f90d",
      "dest_uuid": "1a88f58d-3353-4a55-83be-1d6ab58b2bfc",
   
      "channel_name": "SIP/300-00000014",
      "dial_addr": "300",
      "dial_trycnt": 1,
      "dial_channel": "sip/300",
      "dial_index": 1,
      "dial_data": "sales_1",
      "dial_type": 1,
      "dial_application": "queue",
   
           "tm_dial_begin": "2016-10-24T22:51:27.734721762Z",
      "tm_dial_end":   "2016-10-24T22:51:29.294001808Z"
      "tm_dialing":    "2016-10-24T22:50:10.784443999Z",
      "tm_hangup":     "2016-10-24T22:51:32.482367256Z",
   
      "res_dial": 4,
      "res_hangup": 16,
      "res_hangup_detail": "Normal Clearing",
   
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
      "info_dl_list": {{
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
      }
   }
