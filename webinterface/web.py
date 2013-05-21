#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import os
from datetime import datetime, timedelta
from lock import Lock
from flask import Flask, abort, render_template, send_file, jsonify,\
                  current_app, request, session, redirect, g, url_for,\
                  _request_ctx_stack
from flask.ext.sqlalchemy import SQLAlchemy
from flask.ext.login import LoginManager, current_user, login_required,\
                            login_user, logout_user,\
                            confirm_login, fresh_login_required
from flask.ext.wtf import Form, widgets, TextField, BooleanField, PasswordField,\
                            HiddenInput, IntegerField, SelectMultipleField,\
                            SubmitField, DecimalField, SelectField, validators
from flask.ext.principal import Principal, Permission, Identity, \
                                AnonymousIdentity, identity_changed,\
                                identity_loaded, RoleNeed
from matedealer import *

__version__ = 0.1


##############################################################################
# Settings
##############################################################################

DEBUG = False#True

##############################################################################

app = Flask(__name__)
app.config.from_object(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///matedealer.db'
app.config['SECRET_KEY'] = "thats-my-secret" 
db = SQLAlchemy(app)
lm = LoginManager()
lm.setup_app(app)
principal = Principal(app)
lock = Lock()


permission_admin = Permission(RoleNeed(u'admin'))
permission_treasurer = Permission(RoleNeed(u'treasurer'))
permission_user = Permission(RoleNeed(u'user'))

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
    balance = db.Column(db.Integer())
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

    
class Product(db.Model):

    id = db.Column(db.Integer, primary_key=True)    
    name = db.Column(db.String(64))
    price = db.Column(db.Integer())
    slot = db.Column(db.Integer())
    stock = db.Column(db.Integer())
    alert_level = db.Column(db.Integer())

    def __init__(self, name, price, slot, stock, alert_level):
        self.name = name
        self.price = price
        self.slot = slot
        self.stock = stock
        self.alert_level = alert_level

    def __repr__(self):
        return "<Product ('%s',price='%s',slot='%s',stock='%s',alert='%s')>" % (self.name, self.price, self.slot, self.stock, self.alert_level)

class Vend(db.Model):

    id = db.Column(db.Integer, primary_key=True)    
    user =  db.Column(db.String(64))
    product = db.Column(db.String(64))
    price = db.Column(db.Integer())
    timestamp = db.Column(db.DateTime()) 

    def __init__(self, user, product, price, timestamp):
        self.user = user
        self.product = product
        self.price = price
        self.timestamp = timestamp

    def __repr__(self):
        return "<Vend ('%s','%s','%s','%s')>" % (self.user, self.product, self.price, self.timestamp)

class Payment(db.Model):

    id = db.Column(db.Integer, primary_key=True)    
    treasurer = db.Column(db.String(64))
    user = db.Column(db.String(64))
    amount = db.Column(db.Integer()) 
    timestamp = db.Column(db.DateTime()) 

    def __init__(self, treasurer, user, amount, timestamp):
        self.treasurer = treasurer
        self.user = user
        self.amount = amount
        self.timestamp = timestamp

    def __repr__(self):
        return "<Payment ('%s','%s','%s','%s')>" % (self.treasurer, self.user, self.amount, self.timestamp)


sc = SessionController(db, User, Product, Vend)

##############################################################################
# Form Models 
##############################################################################

class HiddenInteger(IntegerField):
    widget = HiddenInput()

class MultiCheckboxField(SelectMultipleField):
    widget = widgets.ListWidget(prefix_label=False)
    option_widget = widgets.CheckboxInput()

class LoginForm(Form):
    user = TextField('user', validators = [validators.Required()])
    password = PasswordField('password', validators = [validators.Required()])

class AddUserForm(Form):
    user = TextField('user', validators = [validators.Required(), validators.Length(min=3, max=64)])
    email = TextField('email', validators = [validators.Required(), validators.Email()])
    password = PasswordField('password', validators = [validators.Required(), validators.Length(min=3, max=64), validators.EqualTo('confirm','Both passwords must be equal.')])
    confirm = PasswordField('confirm')
    roles = MultiCheckboxField('roles', validators = [validators.Required()], coerce=int)
 
class EditUserForm(Form):
    id = HiddenInteger('id')
    user = TextField('user', validators = [validators.Required(), validators.Length(min=3, max=64)])
    email = TextField('email', validators = [validators.Required(), validators.Email()])
    roles = MultiCheckboxField('roles', validators = [validators.Required()], coerce=int)
 
class DeleteForm(Form):
    id = HiddenInteger('id')
    submit = SubmitField(u'Löschen')
 
class ProductForm(Form):
    name = TextField('name', validators = [validators.Required(), validators.Length(min=3, max=64)])
    price = DecimalField('price', validators = [validators.NumberRange(min=0.1,max=100.0)])
    slot = IntegerField('slot', validators = [validators.Required(), validators.NumberRange(min=1,max=5)])
    stock = IntegerField('stock', validators = [validators.Required()])
    alert_level = IntegerField('alert_level', validators = [validators.Required()])
     
class TreasurerForm(Form):
    user = SelectField('user', coerce=int)     
    amount = IntegerField('amount', validators = [validators.NumberRange(min=-500,max=500)])

##############################################################################
# Routes
##############################################################################

@lm.user_loader
def load_user(id):
    return User.query.get(int(id))


@identity_loaded.connect_via(app)
def on_identity_loaded(sender, identity):
    identity.user = current_user
    if hasattr(current_user, 'roles'):
        for role in current_user.roles:
            identity.provides.add(RoleNeed(unicode(role.role)))


@app.before_request
def before_request():
    g.user = current_user
    g.lock = lock


# only defined to prevent needless filestsystem calls (and keep log clean)
@app.route('/favicon.ico')
def favicon():
    abort(404)


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/login', methods = ['GET', 'POST'])
def login():
    if g.user is not None and g.user.is_authenticated():
        return redirect(url_for('logout'))
    form = LoginForm()
    if g.lock.is_locked():
        if float(g.lock.time_left().replace(':','.')) > 0:
            return render_template('login.html', form=form)
    if form.validate_on_submit():
        user = User.query.filter_by(name=form.user.data, password=form.password.data).first()
        if user is not None:
            #g.lock.lock(user.name)
            login_user(user) 
            identity_changed.send(current_app._get_current_object(),
                identity=Identity(user.id))
            return redirect(url_for('index'))
        else:
            form.errors["loginerror"] = "Login fehlgeschlagen"
            return render_template('login.html', form=form)
    return render_template('login.html', form=form)


@app.route('/vending')
@permission_user.require(401)
def vending():
    return render_template('vending.html')

@app.route('/status')
@permission_user.require(401)
def status():
    print("USER: %s" % sc.user.name)
    return jsonify({"locked" : sc.is_locked(), "user" : sc.user.name })

@app.route('/vend/start')
@permission_user.require(401)
def vend():
    sc.vend_start(g.user.id)
    return jsonify({"vend" : True})

@app.route('/vend/cancel')
@permission_user.require(401)
def vend():
    sc.cancel()
    return jsonify({"cancel" : True})


@app.route('/treasurer', methods=['GET','POST'])
@permission_treasurer.require(401)
def treasurer():
    users = User.query.filter(User.name != "root").all() 
    form = TreasurerForm()
    form.user.choices = [(user.id,user.name) for user in users]
    if form.validate_on_submit():
        user = User.query.get(form.user.data)
        user.balance += int(float(form.amount.data) * 100)
        payment = Payment(g.user.name, user.name, int(float(form.amount.data) * 100), datetime.now())   
        db.session.add(payment)
        db.session.commit()
        return render_template('treasurer.html',form=form, users=users)
    return render_template('treasurer.html',form=form, users=users)


@app.route('/admin')
@permission_admin.require(401)
def admin():
    return render_template('admin.html', 
        userdata=get_user_data(), 
        roles=get_roles(),
        products=get_products(),
        vends=get_vends(),
        payments=get_payments())


@app.route('/add/user', methods=['GET','POST'])
@permission_admin.require(401)
def add_user():
    form = AddUserForm()
    form.roles.choices = [(id, role.capitalize()) for id, role in Role.query.with_entities(Role.id,Role.role).all()]
    form.roles.default = [role.id for role in Role.query.filter_by(role="user").all()]
    if form.validate_on_submit():
        new = User(form.user.data, form.email.data, form.password.data, 0.0, [Role.query.get(id) for id in form.roles.data])
        db.session.add(new)
        db.session.commit()
        return redirect(url_for('admin'))
    if not form.is_submitted():
        form.process() 
    return render_template('add_user.html', form=form) 


@app.route('/edit/user/<int:id>', methods=['GET','POST'])
@permission_admin.require(401)
def edit_user(id):
    user = User.query.get(id)
    form = EditUserForm()
    form.roles.choices = [(id, role.capitalize()) for id, role in Role.query.with_entities(Role.id,Role.role).all()]
    form.roles.default = [role.id for role in user.roles]
    if form.validate_on_submit():
        user.name = form.user.data
        user.email = form.email.data
        user.roles = [Role.query.get(id) for id in form.roles.data]
        db.session.commit()
        return redirect(url_for('admin'))
    if not form.is_submitted():
        form.process() 
        form.id.data = user.id
        form.user.data = user.name
        form.email.data = user.email
    return render_template('edit_user.html', form=form) 


@app.route('/delete/user/<int:id>', methods=['GET','POST'])
@permission_admin.require(401)
def delete_user(id):
    user = User.query.get(id)
    form = DeleteForm()
    if form.validate_on_submit():
        db.session.delete(user)
        db.session.commit()
        return redirect(url_for('admin'))
    return render_template('delete_user.html', user=user.name, form=form) 


@app.route('/add/product', methods=['GET','POST'])
@permission_admin.require(401)
def add_product():
    form = ProductForm()
    if form.validate_on_submit():
        price = int(float(form.price.data) * 100)
        new = Product(form.name.data, price, form.slot.data, form.stock.data, form.alert_level.data)
        db.session.add(new)
        db.session.commit()
        return redirect(url_for('admin'))
    return render_template('add_product.html', form=form) 


@app.route('/edit/product/<int:id>', methods=['GET','POST'])
@permission_admin.require(401)
def edit_product(id):
    product = Product.query.get(id)
    form = ProductForm()
    if form.validate_on_submit():
        product.name = form.name.data
        product.price = form.price.data
        product.slot = form.slot.data
        product.stock = form.stock.data
        product.alert_level = form.alert_level.data
        db.session.commit()
        return redirect(url_for('admin'))
    if not form.is_submitted():
        form.name.data = product.name
        form.price.data = product.price
        form.slot.data = product.slot
        form.stock.data = product.stock
        form.alert_level.data = product.alert_level
    return render_template('add_product.html', form=form) 


@app.route('/delete/product/<int:id>', methods=['GET','POST'])
@permission_admin.require(401)
def delete_product(id):
    product = Product.query.get(id)
    form = DeleteForm()
    if form.validate_on_submit():
        db.session.delete(product)
        db.session.commit()
        return redirect(url_for('admin'))
    return render_template('delete_product.html', product=product.name, form=form) 


@app.route('/logout')
def logout():
    #g.lock.unlock()
    logout_user()
    for key in ('identity.name', 'identity.auth_type'):
        session.pop(key, None)
    identity_changed.send(current_app._get_current_object(),
                          identity=AnonymousIdentity())
    return redirect(url_for('index'))


@app.errorhandler(405)
def page_not_found(error):
    return render_template('405.html'), 405


@app.errorhandler(404)
def page_not_found(error):
    return render_template('404.html'), 404


@app.errorhandler(401)
def permission_denied(error):
    return render_template('401.html'), 401


##############################################################################
# Template filters
##############################################################################
@app.template_filter('cent_to_eur')
def cent_to_eur(value):
    return "%.2f" % (float(value) / 100)

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
def get_chart_data():
    return Vend.query.order_by(product).all()

def get_user_data():
    return User.query.filter(User.name != "root").all() 

def get_roles():
    return [role.role.capitalize() for role in Role.query.all()]

def get_products():
    return Product.query.all()

def get_vends():
    return Vend.query.all()

def get_payments():
    return Payment.query.all()

##############################################################################
# Main
##############################################################################

if __name__ == '__main__':
    app.debug = DEBUG
    # Print version info
    app.logger.debug('App started')
    app.run(host='0.0.0.0')
