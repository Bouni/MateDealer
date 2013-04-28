#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import os
from datetime import datetime, timedelta
from flask import Flask, abort, render_template, send_file, jsonify,\
                  current_app, request, session, redirect, g, url_for
from flask.ext.sqlalchemy import SQLAlchemy
from flask.ext.login import LoginManager, current_user, login_required,\
                            login_user, logout_user,\
                            confirm_login, fresh_login_required
from flask.ext.wtf import Form, TextField, BooleanField, PasswordField,\
                            validators
from flask.ext.principal import Principal, Permission, Identity, \
                                AnonymousIdentity, identity_changed,\
                                identity_loaded, RoleNeed

__version__ = 0.1


##############################################################################
# Settings
##############################################################################

DEBUG = True

##############################################################################

app = Flask(__name__)
app.config.from_object(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///matedealer.db'
app.config['SECRET_KEY'] = os.urandom(24) 
db = SQLAlchemy(app)
pp = Principal(app)
lm = LoginManager()
lm.setup_app(app)

admin_permission = Permission(RoleNeed('admin'))
user_permission = Permission(RoleNeed('user'))

##############################################################################
# Lock Mechanism 
##############################################################################

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
        return "%d:%d" % (minutes, seconds)

##############################################################################
# Database Models 
##############################################################################

# Helper table for many-to-many realtionship of users <> roles
roles = db.Table('roles',
    db.Column('role_id', db.Integer, db.ForeignKey('role.id')),
    db.Column('user_id', db.Integer, db.ForeignKey('user.id'))
)

class User(db.Model):
 
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(64))
    email = db.Column(db.String(64))
    password = db.Column(db.String(64))
    balance = db.Column(db.Float())
    roles = db.relationship('Role', secondary=roles,
        backref=db.backref('users', lazy='dynamic'))


    def __init__(self, name, email, password, balance, roles):
        self.name = name
        self.email = email
        self.password = password
        self.balance = balance
        self.roles = roles

    def is_authenticated(self):
        return True

    def is_active(self):
        return True

    def is_anonymous(self):
        return False

    def get_id(self):
        return unicode(self.id)

    def __repr__(self):
        return "<User ('%s','%s','%s')>" % (self.name, self.email, self.balance)

class Role(db.Model):
    
    id = db.Column(db.Integer, primary_key=True)    
    role = db.Column(db.String(64))

    def __init__(self, role):
        self.role = role    

    def __repr__(self):
        return "<Role ('%s')>" % self.role
    

class Transaction(db.Model):

    id = db.Column(db.Integer, primary_key=True)    
    user_id = db.Column(db.Integer, db.ForeignKey('user.id'))
    user = db.relationship('User', backref=db.backref('users', lazy='dynamic'))
    product_id = db.Column(db.Integer, db.ForeignKey('product.id'))
    product = db.relationship('Product', backref=db.backref('products', lazy='dynamic'))
    timestamp = db.Column(db.DateTime()) 

    def __init__(self, user, product, timestamp):
        self.user = user
        self.product = product
        self.timestamp = timestamp

    def __repr__(self):
        return "<Transaction ('%s','%s','%s')>" % (self.user, self.product, self.timestamp)


class Product(db.Model):

    id = db.Column(db.Integer, primary_key=True)    
    name = db.Column(db.String(64))
    stock = db.Column(db.Integer())
    alert_level = db.Column(db.Integer())

    def __init__(self, name, stock, alert_level):
        self.name = name
        self.stock = stock
        self.alert_level = alert_level

    def __repr__(self):
        return "<Product ('%s','%s','%s')>" % (self.name, self.stock, self.alert_level)

##############################################################################
# Form Models 
##############################################################################

class LoginForm(Form):
    user = TextField('user', validators = [validators.Required()])
    password = PasswordField('password', validators = [validators.Required()])

##############################################################################
# Routes
##############################################################################

@lm.user_loader
def load_user(id):
    return User.query.get(int(id))

@identity_loaded.connect_via(app)
def on_identity_loaded(sender, identity):
    app.logger.debug('on_identity_loaded() called')
    identity.user = current_user
    """
    if hasattr(current_user, 'id'):
        identity.provides.add(UserNeed(current_user.id))
        print("2:",identity)
    """
    if hasattr(current_user, 'roles'):
        for role in current_user.roles:
            identity.provides.add(RoleNeed(role.role))
    print(identity)


@app.before_request
def before_request():
    g.user = current_user
    g.lock = lock

@app.route('/')
def index():
    app.logger.debug('index() called')
    print(g.identity)
    return render_template('index.html')

@app.route('/login', methods = ['GET', 'POST'])
def login():
    app.logger.debug('login() called')
    if g.user is not None and g.user.is_authenticated():
        return redirect(url_for('logout'))
    form = LoginForm()
    if g.lock.is_locked():
        return render_template('login.html', form=form)
    if form.validate_on_submit():
        user = User.query.filter_by(name=form.user.data, password=form.password.data).first()
        if user is not None:
            g.lock.lock(user.name)
            login_user(user)
            identity_changed.send(current_app._get_current_object(),
                identity=Identity(user.id))
            return redirect(url_for('index'))
        else:
            form.errors["loginerror"] = "Login fehlgeschlagen"
            return render_template('login.html', form=form)
    return render_template('login.html', form=form)

@app.route('/vending')
#@login_required
#@user_permission.require()
def vending():
    return "vending"

@app.route('/admin')
#@admin_permission.require()
def admin():
    return "admin"

@app.route('/logout')
def logout():
    g.lock.unlock()
    logout_user()
    for key in ('identity.name', 'identity.auth_type'):
        session.pop(key, None)
    identity_changed.send(current_app._get_current_object(),
                          identity=AnonymousIdentity())
    return redirect(url_for('index'))

@app.errorhandler(404)
def page_not_found(error):
    app.logger.debug('page_not_found() called')
    return render_template('404.html'), 404

@app.errorhandler(401)
def page_not_found(error):
    app.logger.debug('page_not_found() called')
    return render_template('401.html'), 401

##############################################################################
# Template filters
##############################################################################
"""
@app.template_filter('dateformat')
def dateformat(date, format='%d.%m.%Y'):
    return date.strftime(format)

@app.template_filter('current_date')
def current_date(none,format='%d.%m.%Y'):
    return datetime.datetime.now().strftime(format)

@app.context_processor
def utility_processor():
    def current_date(format):
        return datetime.datetime.now().strftime(format)
    def bla():
        return "Bla Bla Bla"
    return dict(current_date=current_date, blubber=bla)
    # jinja2 syntax = {{ blubber() }} -> return "Bla Bla Bla"
"""
#############################################################################r
# Helper functions
##############################################################################

        

def get_versions_info():
    from flask import __version__ as FlaskVersion
    versions =  'web.py Version:      %s\n' % __version__
    versions += 'Flask Version:       %s\n' % FlaskVersion
    return versions

##############################################################################
# Main
##############################################################################

if __name__ == '__main__':
    lock = Lock()
    app.debug = DEBUG
    # Print version info
    if DEBUG:
        app.logger.debug(get_versions_info())
    app.logger.debug('App started')
    app.run(host='0.0.0.0')
