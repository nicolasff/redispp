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

bool
Response::setString(Buffer s) {
	if(m_type != REDIS_STRING) {
		return false;
	}

	m_str = s;
	return true;
}
bool
Response::setLong(long l) {
	if(m_type != REDIS_LONG) {
		return false;
	}

	m_long = l;
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

bool
Response::setBool(bool b) {
	if(m_type != REDIS_BOOL) {
		return false;
	}
	m_bool = b;
	return true;
}

bool
Response::setDouble(double d) {
	if(m_type != REDIS_DOUBLE) {
		return false;
	}
	m_double = d;
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

Buffer
Response::string() const {
	return m_str;
}

string 
Response::str() const {

	std::string ret;
	ret.insert(ret.end(), m_str.begin(), m_str.end());
	return ret;
}

long 
Response::value() const {

	return m_long;
}

bool
Response::boolVal() const {
	return m_bool;
}

double
Response::doubleVal() const {
	return m_double;
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

std::vector<Buffer>
Response::array() const {
	return m_array;
}
RedisMap
Response::map() const {
	return m_map;
}

}

