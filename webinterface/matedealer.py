import sys
import os
import re
from datetime import datetime, timedelta
import time
from serial import Serial
from threading import Thread

class SessionController:

    def __init__(self, db, user, product, vend):
        self.db = db
        self.timer = None
        self._lock = False
        self.product = product
        self.vend = vend
        self.user = user
        self.active_user = ""
        self.md = MateDealer(self)
        self.md.start()
        self.user_id = None

    def is_locked(self):
        return self._lock

    def vend_start(self, user_id):
        self.user_id = user_id
        self.lock()
        user = self.user.query.get(self.user_id)
        self.active_user = user.name 
        self.md.start_session(user.balance)

    def vend_request(self, item):
        product = self.product.query.filter_by(slot=item).first()
        print product
        user = self.user.query.get(self.user_id)
        print user
        if user.balance >= product.price:
            self.md.approve_vend(product.price)
        else:
            self.md.deny_vend()

    def vend_success(self, item):
        product = self.product.query.filter_by(slot=item).first()
        product.stock -= 1
        user = self.user.query.get(self.user_id)
        user.balance -= product.price
        vend = self.vend(user.name, product.name, product.price, datetime.now())
        self.db.session.add(vend)
        self.db.session.commit()
        self.cancel()

    def lock(self):
        self._lock = True

    def unlock(self):
        self._lock = False

    def cancel(self):
        if self.timer != None:
            self.timer.stop()
            self.timer = None
        self.active_user = ""
        self.unlock()
        self.md.cancel_session()


class Timer(Thread):

    def __init__(self, time, callback):
        Thread.__init__(self)
        self.time = time
        self.callback = callback
        self.canceled = False

    def run(self):
        while self.time > 0 and not self.canceled:
            time.sleep(1)
            self.time -= 1

        if not self.canceled:
            self.callback()

    def stop(self):
        self.canceled = True


class MateDealer(Thread):
    _instance = None

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(MateDealer, cls).__new__(cls, *args, **kwargs)
        return cls._instance

    def __init__(self, sc):
        Thread.__init__(self)
        self.sc = sc
        self.serial = Serial()
        self.serial.port = '/dev/ttyACM0'
        self.serial.baudrate = 38400
        self.serial.bytesize = 8
        self.serial.parity = 'N'
        self.serial.stopbits = 1
        self.serial.timeout = 0.1
        self.running = False

    def run(self):
        try:
            self.serial.open()
        except Exception as e:
            print >> sys.stderr, e.message
            return

        print self.serial
        self.running = True
        msg = ''
        while self.running:
            byte = self.serial.read()
            if byte != '':
                msg += byte
            if msg.endswith('\n'):
                self.parse_cmd(msg)
                msg = ''

        if self.serial.is_open():
            self.serial.close()

    def start_session(self, balance):
        self.serial.write('start-session %d\r\n' % balance)
        print 'start-session %d' % balance

    def approve_vend(self, price):
        self.serial.write('approve-vend %d\r\n' % price)
        print 'approve-vend %d\r\n' % price

    def deny_vend(self):
        self.serial.write('deny-vend\r\n')
        print 'deny-vend'

    def cancel_session(self):
        self.serial.write('cancel-session\r\n')
        print 'cancel-session'

    def parse_cmd(self, cmd):
        cmd = cmd.strip()
        if cmd.startswith('vend-request'):
            _cmd, price, item = cmd.split(' ')
            self.sc.vend_request(item)
        elif cmd.startswith('vend-success'):
            _cmd, item = cmd.split(' ')
            self.sc.vend_success(item)
        elif cmd.startswith('session-complete'):
            self.sc.cancel()
        else:
            print cmd

