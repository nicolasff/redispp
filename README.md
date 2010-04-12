This project aims to provide a stable, tested, and feature-complete C++ library for Redis.
It is released under the [New BSD License](http://www.opensource.org/licenses/bsd-license.php).

#### TODO
* Add tests
* Packaging: generate a .so, provide a real Makefile...
* Enable static linking
* Refactor to avoid useless data copies all over the place
* Reconnect on timeouts
* Try on Windows

#### Functions implemented

* get (**TESTED**)
* set (**TESTED**)
* getset (**TESTED**)
* incr (**TESTED**)
* incrby (**TESTED**)
* decr (**TESTED**)
* decrby (**TESTED**)
* rename (**TESTED**)
* renamenx (**TESTED**)
* ttl
* ping (**TESTED**)
* setnx (**TESTED**)
* exists (**TESTED**)
* randomkey
* del (**TESTED**)
* mget
* mset (**NOT ENTIRELY TESTED**)
* msetnx
* expire (**TESTED**)
* expireat (**TESTED**)
* info
* multi
* exec
* discard
* auth
* select
* keys (**TESTED**)
* dbsize
* lastsave
* flushdb
* flushall
* save
* bgsave
* bgrewriteaof
* move
* sort
* type (**TESTED**)
* append
* substr
* config (`get` and `set`, not `resetstat`)

--------------------

* lpush (**TESTED**)
* rpush (**TESTED**)
* rpoplpush (**TESTED**)
* llen (**TESTED**)
* lpop (**TESTED**)
* rpop (**TESTED**)
* blpop
* brpop
* ltrim (**TESTED**)
* lindex (**TESTED**)
* lrem (**TESTED**)
* lset (**TESTED**)
* lrange (**TESTED**)

--------------------

* sadd (**TESTED**)
* srem (**TESTED**)
* sismember (**TESTED**)
* smembers (**TESTED**)
* scard (**TESTED**)
* spop (**TESTED**)
* srandmember
* smove (**TESTED**)
* sinter
* sunion
* sdiff
* sinterstore
* sunionstore
* sdiffstore

--------------------

* zadd
* zincrby
* zrem
* zscore
* zrank
* zrevrank
* zrange
* zrevrange
* zcard
* zcount
* zremrangebyrank
* zrangebyscore
* zremrangebyscore
* zunion
* zinter

--------------------

* hset
* hget
* hdel
* hexists
* hlen
* hkeys
* hvals
* hgetall
* hincrby

#### Functions not implemented

* subscribe
* unsubscribe
* publish
