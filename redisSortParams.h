#ifndef REDIS_SORT_PARAMS_H
#define REDIS_SORT_PARAMS_H

#include "redisCommand.h"

namespace redis {
class SortParams {
public:
	SortParams();
	void by(Buffer pattern);
	void limit(int start, int count);
	void get(Buffer pattern);
	void order(Buffer order);
	void alpha();
	void store(Buffer key);

	Command buildCommand(Buffer key);

private:
	Buffer m_by;
	bool m_has_by;

	int m_limit_start;
	int m_limit_count;
	bool m_has_limit;

	Buffer m_get_pattern;
	bool m_has_get_pattern;

	Buffer m_sort_order;
	bool m_has_sort_order;

	bool m_alpha;

	Buffer m_store;
	bool m_has_store;
};
}

#endif /* REDIS_SORT_PARAMS_H */

