This project aims to provide a stable a feature-complete C++ library for Redis.

#### Functions implemented

* get
* set
* incr
* incrby
* decr
* decrby
* rename
* renameNx
* ttl
* ping
* setnx
* exists
* randomkey
* del
* mget
* mset
* msetnx
* expire
* expireat
* info

--------------------

* lpush
* rpush
* rpoplpush
* llen
* lpop
* rpop
* ltrim
* lindex
* lrem
* lset
* lrange

--------------------

* sadd
* srem
* sismember
* scard
* spop
* srandmember
* smove
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

* auth
* select
* keys
* dbsize
* lastsave
* save
* bgsave
* bgrewriteaof
* shutdown
* move
* type

--------------------

* sync
* flushdb
* flushall
* sort
* monitor
* getset
* slaveof

--------------------

* multi
* exec
* discard
* blpop
* brpop
* append
* substr

--------------------

* config

--------------------

* subscribe
* unsubscribe
* publish
