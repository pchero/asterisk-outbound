.. installation

************
Installation
************

Requirements
============

Asterisk
--------
Asterisk-13.6 or later.

asterisk-zmq
------------
asterisk-zmq-1.5 or later.

Download at here.

::

   https://github.com/pchero/asterisk-zmq/releases
   
Library
-------
Ubuntu/Debian

::

   $ sudo apt-get install 


Install
=======

Module
------
::

   $ make
   $ sudo make install
   $ sudo cp conf/res_outbound.conf /etc/asterisk 

Database
--------

Create new database for the asterisk-outbound.

::

   mysql> create database outbound
   
Run the initial script.

::

   $ cd db_srcripts
   $ mysql -u root -p outbound < create.sql

