#include "redisResponse.h"
#include <string>

using namespace std;
namespace redis {

Response::Response(RedisResponseType t) :
	m_type(t),
	m_long(0),
	m_bool(false)
{

}

// generic setters
template <>
bool
Response::set<Buffer>(Buffer s) {
	if(m_type != REDIS_STRING) {
		return false;
	}

	m_str = s;
	return true;
}

template <>
bool
Response::set<long>(long l) {
	if(m_type != REDIS_LONG) {
		return false;
	}

	m_long = l;
	return true;
}

template <>
bool
Response::set<bool>(bool b) {
	if(m_type != REDIS_BOOL) {
		return false;
	}
	m_bool = b;
	return true;
}

template <>
bool
Response::set<double>(double d) {
	if(m_type != REDIS_DOUBLE) {
		return false;
	}
	m_double = d;
	return true;
}

bool
Response::addString(Buffer s) {
	if(m_type != REDIS_LIST) {
		return false;
	}

	m_array.push_back(s);
	return true;
}

bool
Response::addString(std::string key, std::string val) {
	if(m_type != REDIS_INFO_MAP && m_type != REDIS_HASH) {
		return false;
	}

	return addString(Buffer(key.c_str(), 1+key.size()), Buffer(val.c_str(), 1+val.size()));
}

bool
Response::addString(Buffer key, Buffer val) {
	if(m_type != REDIS_INFO_MAP && m_type != REDIS_HASH) {
		return false;
	}

	m_map.insert(make_pair(key, val));
	return true;
}
bool
Response::addZString(Buffer s, double score) {
	if(m_type != REDIS_ZSET) {
		return false;
	}

	m_zarray.push_back(make_pair(score, s));
	return true;
}

void 
Response::type(RedisResponseType t) {
	m_type = t;
}
RedisResponseType 
Response::type() const {
	return m_type;
}

int
Response::size() const {
	if(m_type == REDIS_LIST) {
		return m_array.size();
	} else if(m_type == REDIS_ZSET) {
		return m_zarray.size();
	} else if(m_type == REDIS_HASH) {
		return m_map.size();
	}
	return -1;
}

// getters
template <>
long Response::get<long>() const {
	return m_long;
}

template <>
const char* Response::get<const char *>() const {
	return &m_str[0];
}

template <>
bool Response::get<bool>() const {
	return m_bool;
}

template <>
double Response::get<double>() const {
	return m_double;
}

template <>
std::vector<Buffer> Response::get<std::vector<Buffer> >() const {
	return m_array;
}

template <>
string Response::get<string>() const {

	std::string ret;
	ret.insert(ret.end(), m_str.begin(), m_str.end());
	return ret;
}

template <>
Buffer Response::get<Buffer>() const {
	return m_str;
}

template <>
RedisMap Response::get<RedisMap>() const {
	return m_map;
}

}

