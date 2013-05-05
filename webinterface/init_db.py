#!/usr/bin/python
# -*- coding: utf-8 -*-
from web import User, Role, Product, Vend, Payment 
from web import db
from datetime import datetime

db.drop_all()
db.create_all()

def add_roles():
    user = Role("user")
    treasurer = Role("treasurer")
    admin = Role("admin")
    db.session.add_all([user,treasurer,admin])
    db.session.commit()

def add_root():
    root = User("root", "matedealer@reaktor23.org", "password", 0, Role.query.all())
    db.session.add(root)
    db.session.commit()

def add_users():
    bouni = User("Bouni", "bouni@owee.de", "password", 0, Role.query.filter(Role.role==u"user").all())
    sannny = User("Samuel", "samuel@owee.de", "password", 0, Role.query.filter(Role.role==u"user").all())
    sniser = User("Sniser", "bouni@owee.de", "password", 0, Role.query.filter(Role.role==u"user").all())
    manuel = User("Manuel", "manuel@owee.de", "password", 0, Role.query.filter(Role.role==u"user").all())
    zeno = User("Zeno", "zeno@owee.de", "password", 0, Role.query.filter(Role.role==u"user").all())
    db.session.add_all([bouni,sannny,sniser,manuel,zeno])
    db.session.commit()

def add_products():
    mate = Product("Club Mate",100,1,0,24)
    mate_cola = Product("Club Mate Cola",100,2,0,24)
    db.session.add_all([mate,mate_cola])
    db.session.commit()

add_roles()

add_root()
add_users()

add_products()
