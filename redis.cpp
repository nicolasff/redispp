#include "redis.h"
#include "redisCommand.h"

#include <iostream>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdlib>

using namespace std;

Redis::Redis() {
}

bool 
Redis::connect(std::string host, short port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(!fd) {
		return false;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr)); // std::fill(_n) ?

	// setup socket
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

	// connect.
	int ret = ::connect(fd, (struct sockaddr*)&addr, sizeof(addr));
	if(ret == 0) {
		m_fd = fd;
		return true;
	}

	return false;
}

void
Redis::run(RedisCommand &c) {

	RedisString cmd = c.get();

	int ret = write(m_fd, &cmd[0], cmd.size());
}

RedisResponse
Redis::read_string() {

	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == '$') {
		int sz = ::atol(str.c_str()+1);
		if(sz == -1) {
			return ret; // not found
		}

		// need to read last \r\n
		char *buf = new char[sz + 2];
		if(sz) {
			read(m_fd, buf, sz + 2);
			RedisString s;
			s.insert(s.end(), buf, buf+sz);
			// set string.
			ret.type(REDIS_STRING);
			ret.setString(s);
		}
		return ret;
	}

	return ret;
}

std::string
Redis::getline() {

	char c;
	string ret;
	while(read(m_fd, &c, 1) != -1) {
		ret.push_back(c);
		if(c == '\n') {
			break;
		}
	}
	return ret;
}

bool
Redis::read_bool() {
	std::string str = getline();
	return (str == "+OK\r\n");
}

RedisResponse
Redis::read_status_code() {
	RedisResponse ret(REDIS_BOOL);

	std::string str = getline();
	if(str[0] == '+') {
		ret.setBool(true);
	}
	return ret;
}
RedisResponse
Redis::read_single_line() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == '+') {
		RedisString s(&str[1]);
		ret.type(REDIS_STRING);
		ret.setString(s);
	}
	return ret;
}

RedisResponse
Redis::read_integer() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == ':') {
		ret.type(REDIS_LONG);
		ret.setLong(::atol(1+&str[0]));
	}
	return ret;
}

/**
 * Reads :1 as true, :0 as false.
 */
RedisResponse
Redis::read_integer_as_bool() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == ':') {
		long l;
		switch((l = ::atol(&str[1]))) {
			case 0:
			case 1:
				ret.type(REDIS_BOOL);
				ret.setBool(l == 1);
				break;
		}
	}
	return ret;
}

RedisResponse
Redis::get(RedisString key){
	RedisCommand cmd("GET");

	cmd << key;
	run(cmd);

	return read_string();
}


bool
Redis::set(const char *key, const size_t key_len, const char *val, const size_t val_len){
	RedisCommand cmd("SET");

	// convert arguments int RedisString
	RedisString k, v;
	k.insert(k.end(), key, key + key_len);
	v.insert(v.end(), val, val + val_len);

	// add the arguments to the command, and run.
	cmd << k << v;
	run(cmd);

	return read_bool();
}
bool 
Redis::set(const std::string key, const std::string val) {
	return set(key.c_str(), key.size(), val.c_str(), val.size());
}

RedisResponse
Redis::setNx(RedisString key, RedisString val) {
	RedisCommand cmd("SETNX");
	cmd << key << val;
	run(cmd);

	return read_integer_as_bool();
}

RedisResponse
Redis::exists(RedisString key) {
	RedisCommand cmd("EXISTS");
	cmd << key;
	run(cmd);

	return read_integer_as_bool();
}

Redis&
Redis::pipeline() {

	if(!m_pipeline) {
		m_pipeline = true;
		// TODO: free.
		m_cmds.clear();
	}
}

RedisResponse 
Redis::incr(RedisString key, int val) {

	return generic_increment("INCR", key, val);
}
RedisResponse 
Redis::decr(RedisString key, int val) {

	return generic_increment("DECR", key, val);
}
RedisResponse
Redis::rename(RedisString src, RedisString dst) {

	RedisCommand cmd("RENAME");
	cmd << src << dst;
	run(cmd);

	return read_status_code();
}

RedisResponse
Redis::renameNx(RedisString src, RedisString dst) {

	RedisCommand cmd("RENAMENX");
	cmd << src << dst;
	run(cmd);

	return read_integer();
}

RedisResponse
Redis::randomKey() {
	RedisCommand cmd("RANDOMKEY");
	run(cmd);

	return read_single_line();
}

RedisResponse
Redis::ttl(RedisString key) {
	RedisCommand cmd("TTL");
	cmd << key;
	run(cmd);

	return read_integer();
}

RedisResponse
Redis::ping() {
	RedisCommand cmd("PING");
	run(cmd);

	return read_status_code();
}

RedisResponse
Redis::generic_increment(std::string keyword, RedisString key, int val) {

	if(val == 1) {
		RedisCommand cmd(keyword);
		cmd << key;
		run(cmd);
	} else {
		RedisCommand cmd(keyword + "BY");
		cmd << key;
		cmd << val;
		run(cmd);
	}

	return read_integer();
}

