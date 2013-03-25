# MateDealer :: a Reaktor23 Hackerspace Project

## General
MateDealer is an open source implementation of a MDB cashless device.
It runs on a Arduino Mega2560 and probably on other Arduinos/ATMEL micro processors.
Its written in c and not in the Arduino language (c++). 

## Known problems
Note: The Arduino Mega2560 by default resets every time you connect to it via serial terminal.

## Setup

  1. Get the repo using `git clone git://github.com/Bouni/MateDealer.git`
  2. Compile it with `make all`
  3. Upload it to your Arduino with `make program`

## Requirements

  - AVR-GCC
  - AVRDUDE
  - avr-libc

## Further information
For more information, see the website at: [Reaktor23 - MateDealer](https://reaktor23.org/de/projects/mate_dealer)

For bugs, contributions, suggestions, or anything else send an E-Mail to:
<info@reaktor23.org>

## Links

[MDB / ICP Specification](http://www.vending.org/technology/MDB_Version_4-2.pdf)
