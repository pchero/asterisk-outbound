.. structure

*********
Structure
*********

Basic structures
================

Plan dial mode
--------------

.. _plan_dial_mode:
.. table:: Plan dial mode

   ==== ==================
   Mode Detail
   ==== ==================
   0    None(No dial mode)
   1    Predictive
   ==== ==================

Campaign status
---------------

.. _campaign_status:
.. table:: Campaign status

   ======   =================
   Status   Detail
   ======   =================
   0        Stop
   1        Start
   2        Pause
   10       Stopping
   11       Starting
   12       Pausing
   30       Stopping forcedly
   ======   =================
   

Dial status
-----------

.. _dial_status:
.. table:: Dial status

   ======   ======
   Status   Detail
   ======   ======
   0        None
   1        Originate request send
   2        Dialing begin
   3        Channel create
   4        Dialing end
   5        Originate response receive
   6        Hangup
   10       Error occurred
   ======   ======


Dial result
-----------

.. _dial_result:
.. table:: Dial result

   ======   =========
   Result   Detail
   ======   =========
   0        Dialing failed
   1        Other end has hungup
   2        Local ring
   3        Remote end is ringing
   4        Remote end has answered
   5        Remote end is busy
   6        Make it go off hook
   7        Line is off hook
   8        Congestion (circuits busy)
   9        Flash hook
   10       Wink
   11       Set a low-level option
   12       Key Radio
   13       Un-Key Radio
   14       Indicate PROGRESS
   15       Indicate CALL PROCEEDING
   16       Indicate call is placed on hold
   17       Indicate call is left from hold
   18       Indicate video frame update
   19       T38 state change request/notification \deprecated This is no longer supported. Use AST_CONTROL_T38_PARAMETERS instead.
   20       Indicate source of media has changed
   21       Indicate status of a transfer request
   22       Indicate connected line has changed
   23       Indicate redirecting id has changed
   24       T38 state change request/notification with parameters
   25       Indication that Call completion service is possible
   26       Media source has changed and requires a new RTP SSRC
   27       Tell ast_read to take a specific action
   28       Advice of Charge with encoded generic AOC payload
   29       Indicate that this position was the end of the channel queue for a softhangup.
   30       Indication that the extension dialed is incomplete
   31       Indicate that the caller is being malicious.
   32       Interrupt the bridge and have it update the peer
   33       Contains an update to the protocol-specific cause-code stored for branching dials
   34       A masquerade is about to begin/end. (Never sent as a frame but directly with ast_indicate_data().)
   ======   =========

Hangup result
-------------

.. _hangup_result:
.. table:: Hangup result

    ======  ====================================
    Result  Detail
    ======  ====================================
    1       Unallocated.
    2       No route trasit network.
    3       No route destination.
    5       Mis dialled trunk prefix.
    6       Channel unacceptable.
    7       Call awarded delivered.
    8       Pre empted.
    14      Number ported not here.
    16      Normal clearing.
    17      User busy.
    18      No user response.
    19      No answer.
    20      Subscriber absent.
    21      Call rejected.
    22      Number changed.
    23      Redirected to new destination.
    26      Answered eleswhere.
    27      Destination out of order.
    28      Invalid number format.
    29      Facility rejected.
    30      Response to status enquiry.
    31      Normal unspecified.
    34      Normal circuit congestion.
    38      Network out of order.
    41      Normal temporary failure.
    42      Switch congestion.
    43      Access info discarded.
    44      Requested chan unavail.
    50      Facility not subscribed.
    52      Outgoing call barred.
    54      Incoming call barred.
    57      Bearer capability not auth.
    58      Bearer capability not available.
    65      Bearer capability not implemented.
    66      Channel not implemented.
    69      Facility not implemented.
    81      Invalid call reference.
    88      Incompatible destination.
    95      Invalid message unspecified.
    96      Madatory item missing.
    97      Message type nonexist.
    98      Wrong message.
    99      Item nonexist.
    100     Invalid item contents.
    101     Wrong call state.
    102     Recovery on timer expire.
    103     Mandatory item length error.
    111     Protocol error.
    127     Internetwroking.
    ======  ====================================
   
   
