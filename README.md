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
* ttl (**TESTED**)
* ping (**TESTED**)
* setnx (**TESTED**)
* exists (**TESTED**)
* randomkey (**TESTED**)
* del (**TESTED**)
* mget (**TESTED**)
* mset (**NOT ENTIRELY TESTED**)
* msetnx
* expire (**TESTED**)
* expireat (**TESTED**)
* info
* multi (**PARTIALLY TESTED**)
* exec (**PARTIALLY TESTED**)
* discard
* auth (**TESTED MANUALLY**)
* select (**TESTED MANUALLY**)
* keys (**TESTED**)
* dbsize (**TESTED**)
* lastsave (**TESTED**)
* flushdb (**TESTED**)
* flushall (**TESTED**)
* save (**TESTED**)
* bgsave (**TESTED**)
* bgrewriteaof (**NOT TESTED**)
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

* zadd (**TESTED**)
* zincrby (**TESTED**)
* zrem (**TESTED**)
* zscore (**TESTED**)
* zrank
* zrevrank
* zrange (**TESTED**)
* zrevrange
* zcard (**TESTED**)
* zcount
* zremrangebyrank
* zrangebyscore (**TESTED**)
* zremrangebyscore (**TESTED**)
* zunion (**TESTED**)
* zinter (**TESTED**)

--------------------

* hset (**TESTED**)
* hget (**TESTED**)
* hdel (**TESTED**)
* hexists (**TESTED**)
* hlen (**TESTED**)
* hkeys (**TESTED**)
* hvals (**TESTED**)
* hgetall (**TESTED**)
* hincrby (**TESTED**)

#### Functions not implemented

* subscribe
* unsubscribe
* publish
