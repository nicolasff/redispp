#ifndef REDIS_STRING_H
#define REDIS_STRING_H

#include <vector>
#include <string>

namespace redis {
class Buffer : public std::vector<char> {

	public:
		Buffer();
		Buffer(std::string &s);
		Buffer(const char *s);
		Buffer(const char *s, size_t sz);

};

typedef std::vector<Buffer> RedisList;

}

#endif /* REDIS_STRING_H */

