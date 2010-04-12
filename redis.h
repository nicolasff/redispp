#ifndef REDIS_H
#define REDIS_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include "redisCommand.h"
#include "redisResponse.h"
#include "redisSortParams.h"

namespace redis {
class Client {

	typedef Response (Client::*ResponseReader)();

public:
	Client();

	static const long ERROR = -1;
	static const long STRING = 1;
	static const long LIST = 2;
	static const long SET = 3;
	static const long ZSET = 4;
	static const long HASH = 5;
	static const long NONE = 6;

	bool connect(std::string host = "127.0.0.1", short port = 6379);
	
	Response auth(Buffer key);
	Response select(int index);
	Response keys(Buffer pattern);
	Response dbsize(int index);
	Response lastsave();
	Response flushdb();
	Response flushall();
	Response save();
	Response bgsave();
	Response bgrewriteaof();
	Response move(Buffer key, int index);
	Response sort(Buffer key);
	Response sort(Buffer key, SortParams params);
	Response type(Buffer key);
	Response append(Buffer key, Buffer padding);
	Response substr(Buffer key, int start, int end);
	Response config(Buffer key, Buffer field);
	Response config(Buffer key, Buffer field, Buffer val);

	Response get(Buffer key);
	Response set(Buffer key, Buffer val);
	Response getset(Buffer key, Buffer val);
	Response incr(Buffer key, int val = 1);
	Response decr(Buffer key, int val = 1);
	Response rename(Buffer src, Buffer dst);
	Response renamenx(Buffer src, Buffer dst);
	Response randomkey();
	Response ttl(Buffer key);
	Response ping();
	Response setnx(Buffer src, Buffer dst);
	Response exists(Buffer key);
	Response del(Buffer key);
	Response del(List key);
	Response mget(List keys);
	Response expire(Buffer key, long ttl);
	Response expireat(Buffer key, long timestamp);
	Response mset(List keys, List vals);
	Response msetnx(List keys, List vals);
	Response info();

	Response lpush(Buffer key, Buffer val);
	Response rpush(Buffer key, Buffer val);
	Response rpoplpush(Buffer src, Buffer dst);
	Response llen(Buffer key);
	Response lpop(Buffer key);
	Response rpop(Buffer key);
	Response blpop(List keys, int timeout);
	Response brpop(List keys, int timeout);
	Response ltrim(Buffer key, int start, int end);
	Response lindex(Buffer key, int pos);
	Response lrem(Buffer key, int count, Buffer val);
	Response lset(Buffer key, int pos, Buffer val);
	Response lrange(Buffer key, int start, int end);

	Response sadd(Buffer key, Buffer val);
	Response srem(Buffer key, Buffer val);
	Response spop(Buffer key);
	Response scard(Buffer key);
	Response sismember(Buffer key, Buffer val);
	Response smembers(Buffer key);
	Response srandmember(Buffer key);
	Response smove(Buffer src, Buffer dst, Buffer member);
	Response sinter(List keys);
	Response sunion(List keys);
	Response sdiff(List keys);
	Response sinterstore(List keys);
	Response sunionstore(List keys);
	Response sdiffstore(List keys);

	Response zadd(Buffer key, double score, Buffer member);
	Response zrem(Buffer key, Buffer member);
	Response zincrby(Buffer key, double score, Buffer member);
	Response zscore(Buffer key, Buffer member);
	Response zrank(Buffer key, Buffer member);
	Response zrevrank(Buffer key, Buffer member);
	Response zrange(Buffer key, long start, long end, bool withscores = false);
	Response zrevrange(Buffer key, long start, long end, bool withscores = false);
	Response zcard(Buffer key);
	Response zcount(Buffer key, long start, long end);
	Response zremrangebyrank(Buffer key, long min, long max);
	Response zremrangebyscore(Buffer key, long min, long max);
	Response zrangebyscore(Buffer key, long min, long max, bool withscores = false);
	Response zrangebyscore(Buffer key, long min, long max, long start, long end, bool withscores = false);

	Response zunion(Buffer key, List keys);
	Response zunion(Buffer key, List keys, std::string aggregate);
	Response zunion(Buffer key, List keys, std::vector<double> weights);
	Response zunion(Buffer key, List keys, std::vector<double> weights, std::string aggregate);

	Response zinter(Buffer key, List keys);
	Response zinter(Buffer key, List keys, std::string aggregate);
	Response zinter(Buffer key, List keys, std::vector<double> weights);
	Response zinter(Buffer key, List keys, std::vector<double> weights, std::string aggregate);

	Response hset(Buffer key, Buffer field, Buffer val);
	Response hget(Buffer key, Buffer field);
	Response hdel(Buffer key, Buffer field);
	Response hexists(Buffer key, Buffer field);
	Response hlen(Buffer key);
	Response hkeys(Buffer key);
	Response hvals(Buffer key);
	Response hgetall(Buffer key);
	Response hincrby(Buffer key, Buffer field, double d);

	bool multi();
	bool pipeline();
	void discard();
	std::vector<Response> exec();

private:
	void run(Command &c);
	Response run(Command &c, ResponseReader fun);

	Response generic_key_int_return_int(std::string keyword, Buffer key, int val, bool addBy = false);
	Response generic_push(std::string keyword, Buffer key, Buffer val);
	Response generic_pop(std::string keyword, Buffer key);
	Response generic_list_item_action(std::string keyword, Buffer key, int n, Buffer val, ResponseReader fun);
	Response generic_set_key_value(std::string keyword, Buffer key, Buffer val);
	Response generic_multi_parameter(std::string keyword, List &keys, ResponseReader fun);
	Response generic_zrank(std::string keyword, Buffer key, Buffer member);
	Response generic_zrange(std::string keyword, Buffer key, long start, long end, bool withscores);
	Response generic_z_start_end_int(std::string keyword, Buffer key, long start, long end);
	Response generic_card(std::string keyword, Buffer key);
	Response generic_z_set_operation(std::string keyword, Buffer key, List keys,
		std::vector<double> weights, std::string aggregate);
	Response generic_mset(std::string keyword, List keys, List vals, ResponseReader fun);
	Response generic_h_simple_list(std::string keyword, Buffer key);
	Response generic_blocking_pop(std::string keyword, List keys, int timeout);

	
	Response read_string();
	Response read_integer();
	Response read_double();
	Response read_integer_as_bool();
	Response read_status_code();
	Response read_single_line();
	Response read_multi_bulk();
	Response read_queued();
	Response read_info_reply();
	Response read_type_reply();
	Response read_key_value_list();

	std::string getline();
	int m_fd;

	// MULTI/EXEC
	bool m_multi;
	std::vector<ResponseReader> m_readers;

	// pipeline
	bool m_pipeline;
	Buffer m_cmd;

};
}


#endif /* REDIS_H */
