#ifndef REDIS_RESPONSE_H
#define REDIS_RESPONSE_H

#include "redisCommand.h"
#include <map>
#include <vector>
#include <string>

typedef enum {REDIS_ERR, REDIS_LONG, REDIS_STRING, REDIS_BOOL, REDIS_INFO_MAP,
	REDIS_DOUBLE, REDIS_LIST, REDIS_ZSET, REDIS_QUEUED} RedisResponseType;

class RedisResponse {

public:
	RedisResponse(RedisResponseType t);
	bool setString(RedisString s);
	bool setLong(long l);
	bool setBool(bool b);
	bool setDouble(double d);
	bool addString(RedisString s);
	bool addString(std::string key, std::string val);
	bool addZString(RedisString s, double score);

	void type(RedisResponseType t);
	RedisResponseType type() const;

	RedisString string() const;
	std::string str() const;
	long value() const;
	bool boolVal() const;
	double doubleVal() const;

	int size() const;

private:
	RedisResponseType m_type;

	// embeded values
	long m_long;
	bool m_bool;
	double m_double;
	RedisString m_str;
	std::vector<RedisString> m_array;
	std::vector<std::pair<double, RedisString> > m_zarray;
	RedisInfoMap m_map;
};

#endif /* REDIS_RESPONSE_H */

