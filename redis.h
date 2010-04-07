#ifndef REDIS_H
#define REDIS_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include "redisCommand.h"
#include "redisResponse.h"

typedef std::vector<RedisString> RedisList;

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

	RedisResponse lpush(RedisString key, RedisString val);
	RedisResponse rpush(RedisString key, RedisString val);
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


private:
	void run(RedisCommand &c);

	RedisResponse generic_increment(std::string keyword, RedisString key, int val);
	RedisResponse generic_push(std::string keyword, RedisString key, RedisString val);
	RedisResponse generic_pop(std::string keyword, RedisString key);
	void          generic_list_item_action(std::string keyword, RedisString key, int n, RedisString val);
	RedisResponse generic_set_key_value(std::string keyword, RedisString key, RedisString val);
	void          generic_multi_parameter(std::string keyword, RedisList &keys);
	
	RedisResponse read_string();
	RedisResponse read_integer();
	RedisResponse read_integer_as_bool();
	RedisResponse read_status_code();
	RedisResponse read_single_line();
	RedisResponse read_multi_bulk();

	std::string getline();
	int m_fd;

	std::list<std::pair<int, char*> > m_cmds;

};


#endif /* REDIS_H */
