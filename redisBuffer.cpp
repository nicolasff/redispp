#include "redisBuffer.h"
#include <cstring>

using namespace std;

namespace redis {

Buffer::Buffer() : vector<char>() {

}
Buffer::Buffer(std::string &s) : vector<char>() {

	insert(end(), s.begin(), s.end());
}
Buffer::Buffer(const char *s) : vector<char>() {

	int l = ::strlen(s);
	insert(end(), s, s+l);
}

Buffer::Buffer(const char *s, size_t sz) : vector<char>() {

	insert(end(), s, s+sz);
}
}

