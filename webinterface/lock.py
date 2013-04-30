from datetime import datetime, timedelta

class Lock():
    
    def __init__(self, locked=False, timeout=60*5):
        self.locked = locked
        self.locker = None
        self.timestamp = datetime.now()
        self.timeout = timeout

    def lock(self, locker):
        self.locker = locker
        self.locked = True

    def unlock(self):
        self.locker =  None
        self.locked = False

    def get_locker(self):
        return self.locker

    def is_locked(self):
        return self.locked
    
    def time_left(self):
        delta = (self.timestamp + timedelta(0,self.timeout)) - datetime.now()
        minutes, seconds = divmod(delta.seconds, 60)
        if minutes > 0 and seconds > 0: 
            return "%d:%d" % (minutes, seconds)
        else:
            return "0:00"
