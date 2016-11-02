BFH carme-m4 can LED exchange
============================

## Synopsis

The following sketch is made for the carme-m4 (STM32) kit of the Bern University of Applied Sciences.
It implements a CAN data exchange between two boards which are connected to the same can bus. 
The Board uses the SJA1000 can chip.

Function overview:
- Board1 has the identifier 0x666 and an acceptance filter for 0x555
- Board2 has the identifier 0x555 and an acceptance filter for 0x666

The boards then exchange their port status and display them.


## Links

- https://carme.bfh.ch
