#include "redisSortParams.h"
#include "redisCommand.h"

namespace redis {

SortParams::SortParams() :
	m_has_by(false),
	m_has_limit(false),
	m_has_get_pattern(false),
	m_has_sort_order(false),
	m_alpha(false),
	m_has_store(false) {

}

void
SortParams::by(Buffer pattern) {
	m_by = pattern;
	m_has_by = true;
}

void
SortParams::limit(int start, int count) {
	m_limit_start = start;
	m_limit_count = count;
	m_has_limit = true;
}

void
SortParams::get(Buffer pattern) {
	m_get_pattern = pattern;
	m_has_get_pattern = true;
}

void
SortParams::order(Buffer order) {
	m_sort_order = order;
	m_has_sort_order = true;
}

void
SortParams::alpha() {
	m_alpha = true;
}

void
SortParams::store(Buffer key) {
	m_store = key;
	m_has_store = true;
}

Command
SortParams::buildCommand(Buffer key) {

	Command cmd("SORT");
	cmd << key;

	if(m_has_by) { // BY pattern
		cmd << Buffer("BY") << m_by;
	}

	if(m_has_limit) { // LIMIT start count
		cmd << Buffer("LIMIT") << (long)m_limit_start << (long)m_limit_count;
	}

	if(m_has_get_pattern) { // GET pattern
		cmd << Buffer("GET") << m_get_pattern;
	}

	if(m_has_sort_order) { // ASC|DESC
		cmd << m_sort_order;
	}

	if(m_alpha) { // ALPHA
		cmd << Buffer("ALPHA");
	}

	if(m_has_store) { // STORE dstkey
		cmd << Buffer("STORE") << m_store;
	}

	return cmd;
}

}

