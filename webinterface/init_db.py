#!/usr/bin/python
# -*- coding: utf-8 -*-
from web import User, Role, Transaction, Product
from web import db
from datetime import datetime

#db.drop_all()
#db.create_all()

def add_roles():
    user = Role("user")
    treasurer = Role("treasurer")
    admin = Role("admin")
    db.session.add_all([user,treasurer,admin])
    db.session.commit()

def add_user():
    root = User("root", "matedealer@reaktor23.org", "password", 0, Role.query.all())
    db.session.add(root)
    db.session.commit()

#add_roles()
#add_user()

r = User.query.first()
for role in r.roles:
    print(role.role)
