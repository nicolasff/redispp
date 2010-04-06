#ifndef REDIS_RESPONSE_H
#define REDIS_RESPONSE_H

#include "redisCommand.h"
#include <map>
#include <vector>
#include <string>

typedef enum {REDIS_ERR, REDIS_LONG, REDIS_STRING,
	REDIS_LIST, REDIS_ZSET} RedisResponseType;

class RedisResponse {

public:
	RedisResponse(RedisResponseType t);
	bool setString(RedisString s);
	bool setLong(long l);
	bool addString(RedisString s);
	bool addZString(RedisString s, double score);

	void type(RedisResponseType t);
	RedisResponseType type() const;

	RedisString string() const;
	std::string str() const;
	long value() const;

private:
	RedisResponseType m_type;

	// embeded values
	long m_long;
	RedisString m_str;
	std::vector<RedisString> m_array;
	std::vector<std::pair<double, RedisString> > m_zarray;
};

#endif /* REDIS_RESPONSE_H */

