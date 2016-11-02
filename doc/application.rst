.. application

***********
Application
***********

OutDlCreate
===========

Syntax
------

::

   OutDlCreate(dlma_uuid,number1[:number2[...]][,name[,detail[,email[,ukey[,var1=val1[:var2=val2]]]]]])

* dlma_uuid : dlma uuid
* number : number
* name : name
* detail : dl detail info
* email : dl email info
* ukey : unique key for dl
* var : variables

Channel variables
-----------------

* OUTSTATUS : Result of application execution.
- SUCCESS : 
- FAILURE : 
* OUTDETAIL : Detail info of OUTSTATUS. If succeess, it sets dl's uuid.

Example
-------

::

   exten => s,n,OutDlCreate(25768502-ef6f-418c-b07b-d2bd1cf31b7e, 301: 302: 303: 304: 305: 306: 307 : 308, test dl client name, test dl client detail, test dl clie@test.com, test dl ukey, var1=val1:var2=val2 : var3 = var3 : var4 = var 4)

   OUTDETAIL=10d3d96d-eecb-45c4-9526-0b90ad613119
   OUTSTATUS=SUCCESS
