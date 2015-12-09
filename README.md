asterisk-outbound
====================

Outbound module for Asterisk.

This module makes easy to managing the outbound calls.

* Managing Campaign.
* Managing Dial list.
* Managing Plan.

## Requirements
* Asterisk-13
* MySQL-5.6

## Installation

### Module
* make
* sudo mv build/res_outbound.so /usr/lib/asterisk/modules/
* sudo cp conf/res_outbound.conf /etc/asterisk/

### Database
* Create new database for Asterisk-outbound
** mysql> create database outbound
* run the init script
** cd db_scripts
** mysql -u root -p outbound < create.sql
 
## License
* BSD License