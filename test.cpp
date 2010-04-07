#include "redis.h"
#include <iostream>

using namespace std;

int main() {

	Redis r;

	r.connect("127.0.0.1", 6379);
	cout << "set('x', 'hello world'): " << (r.set("x", "hello world").boolVal() ? "OK": "FAIL") << endl;
	RedisResponse resp = r.get("x");
	if(resp.type() == REDIS_STRING) {
		cout << "r.get('x') = " << resp.str() << endl;
	} else {
		cout << "FAIL" << endl;
	}

	RedisResponse respx0y = r.incr(RedisString("x\0y", 3));
	if(respx0y.type() == REDIS_LONG) {
		cout << "r.incr('x\\0y') = " << respx0y.value() << endl;
	} else {
		cout << "FAIL" << endl;
	}

	return 0;
}

