#include "redisResponse.h"
#include <string>

using namespace std;

RedisResponse::RedisResponse(RedisResponseType t) :
	m_type(t),
	m_long(0),
	m_bool(false)
{

}

bool
RedisResponse::setString(RedisString s) {
	if(m_type != REDIS_STRING) {
		return false;
	}

	m_str = s;
	return true;
}
bool
RedisResponse::setLong(long l) {
	if(m_type != REDIS_LONG) {
		return false;
	}

	m_long = l;
	return true;
}
bool
RedisResponse::addString(RedisString s) {
	if(m_type != REDIS_LIST) {
		return false;
	}

	m_array.push_back(s);
	return true;
}
bool
RedisResponse::addZString(RedisString s, double score) {
	if(m_type != REDIS_ZSET) {
		return false;
	}

	m_zarray.push_back(make_pair(score, s));
	return true;
}

bool
RedisResponse::setBool(bool b) {
	if(m_type != REDIS_BOOL) {
		return false;
	}
	m_bool = b;
}

void 
RedisResponse::type(RedisResponseType t) {
	m_type = t;
}
RedisResponseType 
RedisResponse::type() const {
	return m_type;
}

RedisString
RedisResponse::string() const {
	return m_str;
}

string 
RedisResponse::str() const {

	std::string ret;
	ret.insert(ret.end(), m_str.begin(), m_str.end());
	return ret;
}

long 
RedisResponse::value() const {

	return m_long;
}

bool
RedisResponse::boolVal() const {
	return m_bool;
}

