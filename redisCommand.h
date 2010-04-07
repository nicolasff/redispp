#ifndef REDIS_COMMAND_H
#define REDIS_COMMAND_H

#include <list>
#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> RedisInfoMap;

class RedisString : public std::vector<char> {

	public:
		RedisString();
		RedisString(std::string &s);
		RedisString(const char *s);
		RedisString(const char *s, size_t sz);

};

typedef std::vector<RedisString> RedisList;

class RedisCommand {

public:
	RedisCommand(std::string keyword);

	RedisCommand &operator<<(const char *s);
	RedisCommand &operator<<(long l);
	RedisCommand &operator<<(double d);
	RedisCommand &operator<<(const RedisString s);

	RedisString get();

private:
	std::list<RedisString> m_elements;
};


#endif /* REDIS_COMMAND_H */
