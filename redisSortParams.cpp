#include "redisSortParams.h"
#include "redisCommand.h"

RedisSortParams::RedisSortParams() :
	m_has_by(false),
	m_has_limit(false),
	m_has_get_pattern(false),
	m_has_sort_order(false),
	m_alpha(false),
	m_has_store(false) {

}

void
RedisSortParams::by(RedisString pattern) {
	m_by = pattern;
	m_has_by = true;
}

void
RedisSortParams::limit(int start, int count) {
	m_limit_start = start;
	m_limit_count = count;
	m_has_limit = true;
}

void
RedisSortParams::get(RedisString pattern) {
	m_get_pattern = pattern;
	m_has_get_pattern = true;
}

void
RedisSortParams::order(RedisString order) {
	m_sort_order = order;
	m_has_sort_order = true;
}

void
RedisSortParams::alpha() {
	m_alpha = true;
}

void
RedisSortParams::store(RedisString key) {
	m_store = key;
	m_has_store = true;
}

RedisCommand
RedisSortParams::buildCommand(RedisString key) {

	RedisCommand cmd("SORT");
	cmd << key;

	if(m_has_by) { // BY pattern
		cmd << RedisString("BY") << m_by;
	}

	if(m_has_limit) { // LIMIT start count
		cmd << RedisString("LIMIT") << (long)m_limit_start << (long)m_limit_count;
	}

	if(m_has_get_pattern) { // GET pattern
		cmd << RedisString("GET") << m_get_pattern;
	}

	if(m_has_sort_order) { // ASC|DESC
		cmd << m_sort_order;
	}

	if(m_alpha) { // ALPHA
		cmd << RedisString("ALPHA");
	}

	if(m_has_store) { // STORE dstkey
		cmd << RedisString("STORE") << m_store;
	}

	return cmd;
}

