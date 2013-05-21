#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import os
from datetime import datetime, timedelta
import time
from serial import Serial
from threading import Thread
from random import randint

class Simulator(Thread):
    
    _instance = None

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(Simulator, cls).__new__(
                                cls, *args, **kwargs)
        return cls._instance

    def __init__(self):
        Thread.__init__(self)
        self.serial             = Serial()
        self.serial.port        = "/dev/pts/6"
        self.serial.baudrate    = 38400
        self.serial.bytesize    = 8
        self.serial.parity      = 'N'
        self.serial.stopbits    = 1
        self.serial.timeout     = 0.1
        self.running            = False    
        self.slot               = None    


    def run(self):
        try:
            self.serial.open()
        except Exception, e:
            print >> sys.stderr, e.message
            return
        print(self.serial)
        self.running = True
        msg = "" 
        while self.running:
            byte = self.serial.read()
            if byte != "":
                msg += byte
            if msg.endswith("\n"):
                self.parse_cmd(msg)       
                msg = "" 
        if self.serial.is_open():
            self.serial.close()

    def session_start_request(self, balance):
        self.slot = None
        print("Session with %d started" % balance)
        self.simulate_vend_request()       

    def simulate_vend_request(self):
        delay = randint(1,20)
        print("Simulate a user vend request in %d seconds" % delay)
        time.sleep(delay)
        self.slot = randint(1,5)
        self.serial.write("vend-request 100 %d\r\n" % self.slot)
        print(">> vend-request 100 %d" % self.slot)

    def vend_approved(self, price):
        print("Simulate a successful vend for %d Cents" % price)
        time.sleep(1)
        self.serial.write("vend-success %d\r\n" % self.slot)
        print(">> vend-success %d" % self.slot)
        time.sleep(1)
        self.simulate_session_complete()
    
    def vend_denied(self):
        time.sleep(1)
        self.simulate_session_complete()

    def simulate_session_complete(self):
        time.sleep(1)
        self.serial.write("session-complete\r\n")
        print(">> session-complete")

    def cancel_session(self):
        time.sleep(1)
        print("session cancel received")
    
    def parse_cmd(self, cmd):
        cmd = cmd.strip()
        if cmd.startswith("start-session"):
            _cmd, balance = cmd.split(" ")
            self.session_start_request(int(balance))
        elif cmd.startswith("approve-vend"):
            _cmd, price = cmd.split(" ")
            self.vend_approved(int(price))
        elif cmd.startswith("deny-vend"):
            self.vend_denied()
        elif cmd.startswith("session-cancel"):
            self.cancel_session()
        else:
            print("Unecpected: %s" % cmd)
                        
if __name__ == '__main__':
    sim = Simulator()
    sim.start()
