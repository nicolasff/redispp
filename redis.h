#ifndef REDIS_H
#define REDIS_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include "redisCommand.h"
#include "redisResponse.h"

class Redis {

public:
	Redis();

	bool connect(std::string host, short port);
	
	RedisResponse get(RedisString key);
	RedisResponse set(RedisString key, RedisString val);
	RedisResponse incr(RedisString key, int val = 1);
	RedisResponse decr(RedisString key, int val = 1);
	RedisResponse rename(RedisString src, RedisString dst);
	RedisResponse renameNx(RedisString src, RedisString dst);
	RedisResponse randomKey();
	RedisResponse ttl(RedisString key);
	RedisResponse ping();
	RedisResponse setNx(RedisString src, RedisString dst);
	RedisResponse exists(RedisString key);
	RedisResponse del(RedisString key);
	RedisResponse del(RedisList key);
	RedisResponse mget(RedisList keys);
	RedisResponse expire(RedisString key, long ttl);
	RedisResponse expireAt(RedisString key, long timestamp);
	RedisResponse mset(RedisList keys, RedisList vals);
	RedisResponse msetnx(RedisList keys, RedisList vals);
	RedisResponse info();

	RedisResponse lpush(RedisString key, RedisString val);
	RedisResponse rpush(RedisString key, RedisString val);
	RedisResponse rpoplpush(RedisString src, RedisString dst);
	RedisResponse llen(RedisString key);
	RedisResponse lpop(RedisString key);
	RedisResponse rpop(RedisString key);
	RedisResponse ltrim(RedisString key, int start, int end);
	RedisResponse lindex(RedisString key, int pos);
	RedisResponse lrem(RedisString key, int count, RedisString val);
	RedisResponse lset(RedisString key, int pos, RedisString val);
	RedisResponse lrange(RedisString key, int start, int end);

	RedisResponse sadd(RedisString key, RedisString val);
	RedisResponse srem(RedisString key, RedisString val);
	RedisResponse spop(RedisString key);
	RedisResponse scard(RedisString key);
	RedisResponse sismember(RedisString key, RedisString val);
	RedisResponse srandmember(RedisString key);
	RedisResponse smove(RedisString src, RedisString dst, RedisString member);
	RedisResponse sinter(RedisList keys);
	RedisResponse sunion(RedisList keys);
	RedisResponse sdiff(RedisList keys);
	RedisResponse sinterstore(RedisList keys);
	RedisResponse sunionstore(RedisList keys);
	RedisResponse sdiffstore(RedisList keys);

	RedisResponse zadd(RedisString key, double score, RedisString member);
	RedisResponse zrem(RedisString key, RedisString member);
	RedisResponse zincrby(RedisString key, double score, RedisString member);
	RedisResponse zscore(RedisString key, RedisString member);
	RedisResponse zrank(RedisString key, RedisString member);
	RedisResponse zrevrank(RedisString key, RedisString member);
	RedisResponse zrange(RedisString key, long start, long end, bool withscores = false);
	RedisResponse zrevrange(RedisString key, long start, long end, bool withscores = false);
	RedisResponse zcard(RedisString key);
	RedisResponse zcount(RedisString key, long start, long end);
	RedisResponse zremrangebyrank(RedisString key, long min, long max);
	RedisResponse zremrangebyscore(RedisString key, long min, long max);
	RedisResponse zrangebyscore(RedisString key, long min, long max, bool withscores = false);
	RedisResponse zrangebyscore(RedisString key, long min, long max, long start, long end, bool withscores = false);

	RedisResponse zunion(RedisString key, RedisList keys);
	RedisResponse zunion(RedisString key, RedisList keys, std::string aggregate);
	RedisResponse zunion(RedisString key, RedisList keys, std::vector<double> weights);
	RedisResponse zunion(RedisString key, RedisList keys, std::vector<double> weights, std::string aggregate);

	RedisResponse zinter(RedisString key, RedisList keys);
	RedisResponse zinter(RedisString key, RedisList keys, std::string aggregate);
	RedisResponse zinter(RedisString key, RedisList keys, std::vector<double> weights);
	RedisResponse zinter(RedisString key, RedisList keys, std::vector<double> weights, std::string aggregate);

	RedisResponse hset(RedisString key, RedisString field, RedisString val);
	RedisResponse hget(RedisString key, RedisString field);
	RedisResponse hdel(RedisString key, RedisString field);
	RedisResponse hexists(RedisString key, RedisString field);


private:
	void run(RedisCommand &c);

	RedisResponse generic_key_int_return_int(std::string keyword, RedisString key, int val, bool addBy = false);
	RedisResponse generic_push(std::string keyword, RedisString key, RedisString val);
	RedisResponse generic_pop(std::string keyword, RedisString key);
	void          generic_list_item_action(std::string keyword, RedisString key, int n, RedisString val);
	RedisResponse generic_set_key_value(std::string keyword, RedisString key, RedisString val);
	void          generic_multi_parameter(std::string keyword, RedisList &keys);
	RedisResponse generic_zrank(std::string keyword, RedisString key, RedisString member);
	RedisResponse generic_zrange(std::string keyword, RedisString key, long start, long end, bool withscores);
	RedisResponse generic_z_start_end_int(std::string keyword, RedisString key, long start, long end);
	RedisResponse generic_card(std::string keyword, RedisString key);
	RedisResponse generic_z_set_operation(std::string keyword, RedisString key, RedisList keys,
		std::vector<double> weights, std::string aggregate);
	bool generic_mset(std::string keyword, RedisList keys, RedisList vals);

	
	RedisResponse read_string();
	RedisResponse read_integer();
	RedisResponse read_double();
	RedisResponse read_integer_as_bool();
	RedisResponse read_status_code();
	RedisResponse read_single_line();
	RedisResponse read_multi_bulk();

	std::string getline();
	int m_fd;

	std::list<std::pair<int, char*> > m_cmds;

};


#endif /* REDIS_H */
