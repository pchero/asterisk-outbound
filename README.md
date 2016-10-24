asterisk-outbound
====================

Outbound module for Asterisk.

This module makes easy to managing the outbound calls.

* Separated/Dynamical resource management(Campaign, Plan, Dial list, Destination).
* Dynamical outbound dial controlling.
* Strategical dialing management.
* AMI Action/Event messaging support.
* Asterisk CLI support.

## Requirements
* Asterisk-13.6
* Asterisk-manager module
* Sqlite3
* Libevent2

## Manual
* https://rawgit.com/pchero/asterisk-outbound/master/doc/_build/html/index.html

## License
* BSD License

## Note
* Fri 11 Dec 2015
* *Increase Asterisk requirements Asterisk-13 to Asterisk-13.6s
* *Asterisk-13.5 has a bug. It doesn't give back unique id for OriginateResponse. 
* *https://issues.asterisk.org/jira/browse/ASTERISK-14125
