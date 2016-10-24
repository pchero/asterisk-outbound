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
* Will be supported in a future.

SMS
+++
* Send an SMS messages
* Will be supported in a future.

Fax
+++
* Send a fax
* Will be supported in a future.


Destination
===========
Determine who get a call after answer(Who).

Normaly, the destination suppose to be an agent. But in the asterisk system, the destination could be anything. For example, it could be extension or application(queue).


