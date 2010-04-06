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
Redis::read_int() {
	string str = getline();
	if(str[0] == ':') {
		RedisResponse ret(REDIS_LONG);
		ret.setLong(::atol(1+&str[0]));
		return ret;
	} else {
		RedisResponse ret(REDIS_ERR);
		return ret;
	}
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

	return read_int();
}

