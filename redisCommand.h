#ifndef REDIS_COMMAND_H
#define REDIS_COMMAND_H

#include "redisBuffer.h"
#include <list>
#include <map>
#include <string>
#include <vector>


namespace redis {

typedef std::map<Buffer, Buffer> RedisMap;

class Command {

public:
	Command(std::string keyword);

	Command &operator<<(const char *s);
	Command &operator<<(long l);
	Command &operator<<(double d);
	Command &operator<<(const Buffer s);

	Buffer get();

private:
	std::list<Buffer> m_elements;
};
}


#endif /* REDIS_COMMAND_H */
