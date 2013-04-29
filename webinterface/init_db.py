#!/usr/bin/python
# -*- coding: utf-8 -*-
from web import User, Role, Transaction, Product
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

def add_treasurer():
    treasurer = User("treasurer", "treasurer@reaktor23.org", "password", 0, Role.query.filter((Role.role==u"treasurer") | (Role.role==u"user")).all())
    db.session.add(treasurer)
    db.session.commit()

def add_user():
    user = User("sniser", "sniser@reaktor23.org", "password", 0, Role.query.filter((Role.role==u"user")).all())
    db.session.add(user)
    db.session.commit()

add_roles()

add_root()
add_treasurer()
add_user()
