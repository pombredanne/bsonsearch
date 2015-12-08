Acknowledgement
===============

Many thanks to Christian Hergert and A. Jesse Jiryu Davis from MongoDB Inc for creating and maintaining the minimized matcher code from the mongo-c-driver.


I've moved the matcher codebase into this repo rather than link to the driver as MongoDB will rightly [remove the c matcher](https://jira.mongodb.org/browse/CDRIVER-955).


This repo continues the effort for a minimally functional and complimentary bson matching engine.


The officially supported [server matching engine](https://github.com/mongodb/mongo/tree/master/src/mongo/db/matcher) will always be far superior option.



bsonsearch
==========

shared object to perform mongodb-like queries against raw bson rather than through a mongod


install on centos (el7) with RPMs

http://pkgs.bauman.in/el7/repoview/python-bsonsearch.html

or add from repo


```
    [pkgs.bauman.in]
    name=Packaged tools for RHEL7 (centos)
    type=rpm-md
    baseurl=http://pkgs.bauman.in/el7/
    gpgcheck=1
    gpgkey=http://pkgs.bauman.in/el7/repodata/RPM-GPG-KEY-Bauman
    enabled=1
```

yum install python-bsonsearch


compile
========

runtime requires

libbson (https://github.com/mongodb/libbson)

libpcre

compilation also requires

libbson-devel

pcre-devel

uthash-devel



```
    gcc -Wall $(pkg-config --cflags --libs libbson-1.0) -lpcre -shared -o libbsoncompare.so -fPIC bsoncompare.c mongoc-matcher.c mongoc-matcher-op.c
```


Usage
==========

The spec parameter supports all query operators (http://docs.mongodb.org/manual/reference/operator/query/) supported by the mongo-c-driver

Currently, that includes $in, $nin, $eq, $neq, $gt, $gte, $lt, and $lte. (See full documentation http://api.mongodb.org/c/current/mongoc_matcher_t.html)

comparison value in spec can be utf8 string, int/long, regex


``` python
    from bsonsearch import bsoncompare
    import bson
    bc = bsoncompare()
    b = {"a":{"$gte":1}}
    c=[ {"a":0},{"a":1},{"a":2}]#dict
    c2=[bson.BSON.encode(x) for x in c] #already bson
    matcher = bc.generate_matcher(b)
    print [bc.match(matcher, x) for x in c]
    print [bc.match(matcher, x) for x in c2]
    bc.destroy_matcher(matcher)
```


If the document contains lists within the namespace, libbson cannot handle queries like mongodb server.

User needs to call the convert_to_and function to build an appropriate spec for the document.
    convert_to_and(spec, doc_id)


Optionally, the user may call destory_matcher following the match, or choose to wait and clear old matchers at a later time.

ipython notebook

``` python
    import bsonsearch
    bc = bsonsearch.bsoncompare()
    doc = {'a': [{'b': [1, 2]}, {'b': [3, 5]}],
           "c":{"d":"dan"}}
    doc_id = bc.generate_doc(doc)
    spec = {"a.b":{"$in":[7, 6, 5]},
            "c.d":"dan"}
    query = bc.convert_to_and(spec, doc_id)
    print query
    matcher = bc.generate_matcher(query)
    print bc.match_doc(matcher, doc_id)
    bc.destroy_doc(doc_id)
    bc.destroy_doc(bc.docs)
    bc.destroy_matcher(bc.matchers)

    >>> {'$and': [{'$or': [{'c.d': 'dan'}]}, {'$or': [{'a.0.b.0': {'$in': [7, 6, 5]}}, {'a.0.b.1': {'$in': [7, 6, 5]}}, {'a.1.b.0': {'$in': [7, 6, 5]}}, {'a.1.b.1': {'$in': [7, 6, 5]}}]}]}
    >>> True
```



streaming example


this example uses KeyValueBSONInput (https://github.com/bauman/python-bson-streaming)

raw bson file containing 7GB of metadata from the enron data set

This amounts to nothing more than a full table scan through a single mongod but can be multiplexed across multiple bson files with multiple processes using tools like xargs


``` python
    from bsonstream import KeyValueBSONInput
    from bsonsearch import bsoncompare
    bc = bsoncompare()
    b = {"a":{"$gte":1}}
    matcher = bc.generate_matcher(b)
    f = open("/home/dan/enron/enron.bson", 'rb')
    stream = KeyValueBSONInput(fh=f, decode=False)
    for doc in stream:
        print bc.match(matcher, doc)
    f.close()
    bc.destroy_matcher(matcher)
```


