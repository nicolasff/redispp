#ifndef REDIS_H
#define REDIS_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include "redisCommand.h"
#include "redisResponse.h"

class Redis {

public:
	Redis();

	bool connect(std::string host, short port);
	
	
	RedisResponse get(RedisString key);

	bool set(const char *key, const size_t key_len, const char *val, const size_t val_len);
	bool set(const std::string key, const std::string val);

	RedisResponse incr(RedisString key, int val = 1);
	RedisResponse decr(RedisString key, int val = 1);
	RedisResponse rename(RedisString src, RedisString dst);
	RedisResponse renameNx(RedisString src, RedisString dst);
	RedisResponse randomKey();
	RedisResponse ttl(RedisString key);
	RedisResponse ping();
	RedisResponse setNx(RedisString src, RedisString dst);

	Redis& pipeline();

private:
	void run(RedisCommand &c);

	RedisResponse generic_increment(std::string keyword, RedisString key, int val);
	
	RedisResponse read_string();
	RedisResponse read_integer();
	RedisResponse read_integer_as_bool();
	RedisResponse read_status_code();
	RedisResponse read_single_line();

	bool read_bool();

	std::string getline();
	int m_fd;

	bool m_pipeline;

	std::list<std::pair<int, char*> > m_cmds;

};


#endif /* REDIS_H */
