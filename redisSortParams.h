#ifndef REDIS_SORT_PARAMS_H
#define REDIS_SORT_PARAMS_H

#include "redisCommand.h"

class RedisSortParams {
public:
	RedisSortParams();
	void by(RedisString pattern);
	void limit(int start, int count);
	void get(RedisString pattern);
	void order(RedisString order);
	void alpha();
	void store(RedisString key);

	RedisCommand buildCommand(RedisString key);

private:
	RedisString m_by;
	bool m_has_by;

	int m_limit_start;
	int m_limit_count;
	bool m_has_limit;

	RedisString m_get_pattern;
	bool m_has_get_pattern;

	RedisString m_sort_order;
	bool m_has_sort_order;

	bool m_alpha;

	RedisString m_store;
	bool m_has_store;
};

#endif /* REDIS_SORT_PARAMS_H */

