FloatOnElectronics
==================

Hardware project around raspberry pi and arduino to collect useful environmental and float tank data to improve and add to the float experience as well as reduce cost and operating labor of the room.

Motivation
----------

Instrumenting the float tank and surrounding room has the potential for a multitude of labor and cost saving measures, as well as some very unique and interesting interactive experiences that enhance and expand the floating experience.

System Architecture
===================

Sensing
-------

A suite of sensors are deployed into the environment to gather various useful sets of data. 

Aggregation
-----------

This disperate data is aggregated and packed up into a standard message and interval.

Reporting
---------

The packetized data is unpacked, verified, enhanced with further metadata and then sent over the network to a remote REST endpoint.

System Implementation
=====================

Sensing
-------

- Flow rate sensor. Connected inline with shower water supply. Pulses for water flow.
- [Future] Submersible temperature sensor. Two wire output.
- [Future] Ambient light sensor. Analog output.

Aggregation
-----------

An arduino collects various sensor types and rolls it up into a packet that it reports via serial to the raspberry pi.

Reporting
---------

The raspberry pi verifies that the data is good, and adds further information (timestamp) before POSTing it the the Helm rest endpoint. This is accomplished with a python program.

Config Settings for Reporter
============================

TODO



