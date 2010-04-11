#include "redis.h"
#include <iostream>
#include <string>

int tests_passed = 0;
int tests_failed = 0;

#define assert(cond) do {if(!(cond)) { \
		cout << "F"; \
		cout << endl << "Test failed at line " << __LINE__ << endl; \
		tests_failed++; \
		cerr.flush(); \
		} else { \
		cout << ".";\
		tests_passed++; \
		}\
		cout.flush(); \
		} while(0)

using namespace std;

void
testPing(redis::Client &redis) {

	redis::Response r = redis.ping();
	assert(r.type() == REDIS_BOOL);
	assert(r.boolVal() == true);
}
void
test1000(redis::Client &redis, int count) {
	char *buf = new char[count+1];
	buf[count] = 0;
	fill(buf, buf + count, 'A');

	redis::Response rSet = redis.set("x", redis::Buffer(buf, count));
	assert(rSet.type() == REDIS_BOOL && rSet.boolVal() == true);

	redis::Response rGet = redis.get("x");
	assert(rGet.type() == REDIS_STRING && rGet.str() == string(buf, count));

	delete[] buf;
}

void
testErr(redis::Client &redis) {

	redis.set("x", "-ERR");
	redis::Response rGet = redis.get("x");
	assert(rGet.type() == REDIS_STRING && rGet.str() == "-ERR");
}

void
testSet(redis::Client &redis) {
	redis::Response ret(REDIS_ERR);

	// set value to "nil"
	ret = redis.set("x", "nil");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.str() == "nil");

	// get twice
	ret = redis.set("x", "val");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.str() == "val");
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.str() == "val");

	// set, delete, and get → not found.
	ret = redis.set("x", "val");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	redis.del("x");
	ret = redis.get("x");
	assert(ret.type() == REDIS_ERR);

	// binary data
	redis::Buffer val("v\0a\0l", 5);
	ret = redis.set("x", val);
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.string() == val);
	
	// binary key
	redis::Buffer k("v\0a\0l", 5);
	ret = redis.set(k, "ok");
	ret = redis.get(k);
	assert(ret.type() == REDIS_STRING && ret.str() == "ok");
	
	// binary key and value
	ret = redis.set(k, val);
	ret = redis.get(k);
	assert(ret.type() == REDIS_STRING && ret.string() == val);

	string keyRN = "abc\r\ndef";
	string valRN = "val\r\nxyz";
	ret = redis.set(keyRN, valRN);
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	ret = redis.get(keyRN);
	assert(ret.type() == REDIS_STRING && ret.str() == valRN);
}

void
testGetSet(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.getset("key", "42");
	assert(ret.type() == REDIS_ERR);

	ret = redis.getset("key", "123");
	assert(ret.type() == REDIS_STRING && ret.str() == "42");

	ret = redis.getset("key", "123");
	assert(ret.type() == REDIS_STRING && ret.str() == "123");
}

void
testRandomKey(redis::Client &redis) {

	// FIXME: this test is disabled, waiting for antirez to close redis issue #88:
	// http://code.google.com/p/redis/issues/detail?id=88#c6

	return;

	for(int i = 0; i < 1000; ++i) {
		redis::Response ret = redis.randomkey();
		cout << ret.str() << endl;
		assert(ret.type() == REDIS_STRING);

		ret = redis.exists(ret.string());
		assert(ret.type() == REDIS_BOOL);
		assert(ret.boolVal());
	}
}

void
testRename(redis::Client &redis) {

	// strings
	redis.del("key0");
	redis.set("key0", "val0");
	redis.rename("key0", "key1");
	assert(redis.get("key0").type() == REDIS_ERR);
	assert(redis.get("key1").str() == "val0");

	// lists
	redis.del("key0");
	redis.lpush("key0", "val0");
	redis.lpush("key0", "val1");
	redis.rename("key0", "key1");
	assert(redis.lrange("key0", 0, -1).size() == 0);

	redis::Response ret = redis.lrange("key1", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.array()[0][0] == string("val1"));
	assert(&ret.array()[1][0] == string("val0"));
}

void
testRenameNx(redis::Client &redis) {

	// strings
	redis.del("key0");
	redis.set("key0", "val0");
	redis.set("key1", "val1");
	redis::Response ret = redis.renamenx("key0", "key1");
	assert(ret.type() == REDIS_BOOL && ret.boolVal() == false);
	ret = redis.get("key0");
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");
	ret = redis.get("key1");
	assert(ret.type() == REDIS_STRING && ret.str() == "val1");

	// lists
	redis.del("key0");
	redis.del("key1");
	redis.lpush("key0", "val0");
	redis.lpush("key0", "val1");
	redis.lpush("key1", "val1-0");
	redis.lpush("key1", "val1-1");
	redis.renamenx("key0", "key1");

	ret = redis.lrange("key0", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.array()[0][0] == string("val1"));
	assert(&ret.array()[1][0] == string("val0"));

	ret = redis.lrange("key1", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.array()[0][0] == string("val1-1"));
	assert(&ret.array()[1][0] == string("val1-0"));

	redis.del("key2");
	redis.renamenx("key0", "key2");
	ret = redis.lrange("key0", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 0);

	ret = redis.lrange("key2", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.array()[0][0] == string("val1"));
	assert(&ret.array()[1][0] == string("val0"));
}



int main() {

	redis::Client r;

	r.connect(); // default settings

	testPing(r);
	test1000(r, 1000);
	test1000(r, 1000000);
	testErr(r);
	testSet(r);
	testGetSet(r);
	testRandomKey(r);
	testRename(r);
	testRenameNx(r);


	cout << endl << tests_passed << " tests passed, " << tests_failed << " failed." << endl;

	return 0;
}

