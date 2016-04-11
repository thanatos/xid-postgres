===================
XIDs for PostgreSQL
===================

First, see `XID <https://github.com/SixArm/sixarm_ruby_xid>`_. Roughly
speaking, a XID is a 128-bit random number forming an ID. It is much like a
UUIDv4, minus a lot of baggage.

This is an implementation of XIDs for PostgreSQL. That is, this allows you to
do:

.. code:: sql

   CREATE TABLE things (
       id xenoid PRIMARY KEY,
       some_attribute varchar(20),
       some_other_attribute integer
       -- etc.
   );

The type in PostgreSQL is named ``xenoid`` because PostgreSQL uses ``xid`` to
mean "transaction ID". (Though by changing the SQL required to create it, i.e.,
by modifying the ``CREATE TYPE`` statement, you can name it whatever you like.)
