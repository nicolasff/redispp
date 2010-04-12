#ifndef REDIS_RESPONSE_H
#define REDIS_RESPONSE_H

#include "redisCommand.h"
#include <map>
#include <vector>
#include <string>

typedef enum {REDIS_ERR, REDIS_LONG, REDIS_STRING, REDIS_BOOL, REDIS_INFO_MAP, REDIS_HASH,
	REDIS_DOUBLE, REDIS_LIST, REDIS_ZSET, REDIS_QUEUED} RedisResponseType;

namespace redis {
class Response {

public:
	Response(RedisResponseType t);
	bool setString(Buffer s);
	bool setLong(long l);
	bool setBool(bool b);
	bool setDouble(double d);
	bool addString(Buffer s);
	bool addString(std::string key, std::string val);
	bool addString(Buffer key, Buffer val);
	bool addZString(Buffer s, double score);

	void type(RedisResponseType t);
	RedisResponseType type() const;

	Buffer string() const;
	RedisMap map() const;

	template <typename T>
	T get() const;
	int size() const;

private:
	RedisResponseType m_type;

	// embeded values
	long m_long;
	bool m_bool;
	double m_double;
	Buffer m_str;
	std::vector<Buffer> m_array;
	std::vector<std::pair<double, Buffer> > m_zarray;
	RedisMap m_map;
};
}

#endif /* REDIS_RESPONSE_H */

