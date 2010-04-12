#include "redis.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>

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

void
testMGet(redis::Client &redis) {
	redis.del("k1");
	redis.del("k2");
	redis.del("k3");

	redis.set("k1", "v1");
	redis.set("k2", "v2");
	redis.set("k3", "v3");

	// mget(array("k1")) == array("v1")
	redis::List l;
	l.push_back(redis::Buffer("k1"));
	redis::Response ret = redis.mget(l);
	assert(ret.type() == REDIS_LIST && ret.size() == 1);
	assert(::memcmp(&ret.array()[0][0], "v1", 2) == 0);

	l.clear();
	l.push_back(redis::Buffer("k1"));
	l.push_back(redis::Buffer("k2"));
	l.push_back(redis::Buffer("k3"));
	ret = redis.mget(l);
	assert(ret.type() == REDIS_LIST && ret.size() == 3);
	assert(::memcmp(&ret.array()[0][0], "v1", 2) == 0);
	assert(::memcmp(&ret.array()[1][0], "v2", 2) == 0);
	assert(::memcmp(&ret.array()[2][0], "v3", 2) == 0);

	// TODO: test with an invalid key.
	l.clear();
	l.push_back(redis::Buffer("k1"));
	l.push_back(redis::Buffer("k3"));
	l.push_back(redis::Buffer("NoKey"));
	ret = redis.mget(l);
}


void
testExpire(redis::Client &redis) {

	redis.del("key");
	redis.set("key", "value");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "value");

	redis.expire("key", 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "value");
	sleep(2);

	ret = redis.get("key");
	assert(ret.type() == REDIS_ERR);
}

void
testExpireAt(redis::Client &redis) {

	redis.del("key");
	redis.set("key", "value");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "value");

	time_t now = time(0);
	redis.expireat("key", now + 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "value");
	sleep(2);

	ret = redis.get("key");
	assert(ret.type() == REDIS_ERR);
}

void
testSetNX(redis::Client &redis) {

	redis.set("key", "42");
	redis::Response ret = redis.setnx("key", "err");
	assert(ret.type() == REDIS_BOOL && !ret.boolVal());

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "42");

	redis.del("key");
	ret = redis.setnx("key", "42");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "42");
}

void
testIncr(redis::Client &redis) {

	redis.set("key", "0");
	redis.incr("key");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "1");
	redis.incr("key");

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "2");

	redis.incr("key", 3);
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "5");

	// increment a non-numeric string
	redis.del("key");
	redis.set("key", "abc");
	redis.incr("key");
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "1");

	redis.incr("key");
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "2");
}

void
testDecr(redis::Client &redis) {

	redis.set("key", "5");
	redis.decr("key");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "4");
	redis.decr("key");

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "3");

	redis.decr("key", 2);
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "1");
}

void
testExists(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.exists("key");
	assert(ret.type() == REDIS_BOOL && ret.boolVal() == false);

	redis.set("key", "value");
	ret = redis.exists("key");
	assert(ret.type() == REDIS_BOOL && ret.boolVal() == true);
}

void
testKeys(redis::Client &redis) {

	for(int i = 0; i < 10; ++i) {
		stringstream s;
		s << "keys-pattern-" << i;
		string str = s.str();
		redis.set(str.c_str(), "test");
	}
	redis.del("keys-pattern-3");

	redis::Response ret = redis.keys("keys-pattern-*");
	assert(ret.type() == REDIS_LIST);
	redis.set("keys-pattern-3", "test");
	redis::Response ret2 = redis.keys("keys-pattern-*");
	assert(ret2.type() == REDIS_LIST);
	assert(ret2.size() == 1+ret.size() && ret.size() == 9);

	stringstream s;
	for(int i = 0; i < 10; ++i) {
		s << ::rand();
	}

	string str = s.str();
	ret = redis.keys(str.c_str());
	assert(ret.type() == REDIS_LIST && ret.size() == 0);
}

void
testDelete(redis::Client &redis) {

	redis.set("key", "val");
	redis::Response ret = redis.del("key");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	// multiple, all existing
	redis.set("x", "0");
	redis.set("y", "1");
	redis.set("z", "2");
	redis::List l;
	l.push_back("x");
	l.push_back("y");
	l.push_back("z");
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.value() == 3);

	// multiple, none existing
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.value() == 0);
	
	// multiple, some existing
	redis.set("y", "1");
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	redis.set("x", "0");
	redis.set("y", "1");
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.value() == 2);
}

void
testType(redis::Client &redis) {

	redis.set("key", "val");
	redis::Response ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.value() == redis::Client::STRING);

	redis.del("key");
	redis.lpush("key", "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.value() == redis::Client::LIST);

	redis.del("key");
	redis.sadd("key", "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.value() == redis::Client::SET);

	redis.del("key");
	redis.zadd("key", 42, "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.value() == redis::Client::ZSET);

	redis.del("key");
	redis.hset("key", "field", "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.value() == redis::Client::HASH);

	redis.del("key");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.value() == redis::Client::NONE);
}

void
testLPushLPop(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.lpush("key", "val0");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);
	ret = redis.lpush("key", "val1");
	assert(ret.type() == REDIS_LONG && ret.value() == 2);
	ret = redis.lpush("key", "val2");
	assert(ret.type() == REDIS_LONG && ret.value() == 3);

	ret = redis.lpop("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");
	ret = redis.lpop("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "val1");
	ret = redis.lpop("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");
	ret = redis.lpop("key");
	assert(ret.type() == REDIS_ERR);

	// binary push
	ret = redis.lpush("key", redis::Buffer("v\0a\0l\0", 6));
	assert(ret.type() == REDIS_LONG && ret.value() == 1);
	ret = redis.lpop("key");
	string s = ret.str();
	assert(ret.type() == REDIS_STRING && ::memcmp(s.c_str(),"v\0a\0l\0", 6) == 0);
}

void
testRPushRPop(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.rpush("key", "val0");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);
	ret = redis.rpush("key", "val1");
	assert(ret.type() == REDIS_LONG && ret.value() == 2);
	ret = redis.rpush("key", "val2");
	assert(ret.type() == REDIS_LONG && ret.value() == 3);

	ret = redis.rpop("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");
	ret = redis.rpop("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "val1");
	ret = redis.rpop("key");
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");
	ret = redis.rpop("key");
	assert(ret.type() == REDIS_ERR);

	// binary push
	ret = redis.rpush("key", redis::Buffer("v\0a\0l\0", 6));
	assert(ret.type() == REDIS_LONG && ret.value() == 1);
	ret = redis.rpop("key");
	string s = ret.str();
	assert(ret.type() == REDIS_STRING && ::memcmp(s.c_str(),"v\0a\0l\0", 6) == 0);
}

void
testLlen(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "val");

	redis::Response ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	redis.lpush("list", "val2");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 2);

	ret = redis.lpop("list");
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	ret = redis.lpop("list");
	assert(ret.type() == REDIS_STRING && ret.str() == "val");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 0); // empty list is 0

	redis.del("list");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 0); // non-existent list is 0

	redis.set("list", "actually not a list");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_ERR); // not a list: error
}

void
testLindex(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "val0");
	redis.lpush("list", "val1");
	redis.lpush("list", "val2");

	redis::Response ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");

	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "val1");

	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");

	ret = redis.lindex("list", 3);
	assert(ret.type() == REDIS_ERR);

	ret = redis.lindex("list", -1);
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");

	ret = redis.lindex("list", -2);
	assert(ret.type() == REDIS_STRING && ret.str() == "val1");

	ret = redis.lindex("list", -3);
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");

	ret = redis.lindex("list", -4);
	assert(ret.type() == REDIS_ERR);
}

void
testLrem(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "a");
	redis.lpush("list", "b");
	redis.lpush("list", "c");
	redis.lpush("list", "c");
	redis.lpush("list", "b");
	redis.lpush("list", "c");
	// ['c', 'b', 'c', 'c', 'b', 'a']
	redis::Response ret = redis.lrem("list", 2, "b");
	// ['c', 'c', 'c', 'a']
	assert(ret.type() == REDIS_LONG && ret.value() == 2); // deleted 2 “b”s from the start
	ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.str() == "c");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "c");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.str() == "c");
	ret = redis.lindex("list", 3);
	assert(ret.type() == REDIS_STRING && ret.str() == "a");

	redis.del("list");
	redis.lpush("list", "a");
	redis.lpush("list", "b");
	redis.lpush("list", "c");
	redis.lpush("list", "c");
	redis.lpush("list", "b");
	redis.lpush("list", "c");
	// ['c', 'b', 'c', 'c', 'b', 'a']
	ret = redis.lrem("list", -2, "c");
	// ['c', 'b', 'b', 'a']
	assert(ret.type() == REDIS_LONG && ret.value() == 2); // deleted 2 “c”s from the end.
	ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.str() == "c");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "b");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.str() == "b");
	ret = redis.lindex("list", 3);
	assert(ret.type() == REDIS_STRING && ret.str() == "a");

	// remove each element
	ret = redis.lrem("list", 0, "a");
	assert(ret.type() == REDIS_LONG && ret.value() == 1); // deleted 1 “a”
	ret = redis.lrem("list", 0, "x");
	assert(ret.type() == REDIS_LONG && ret.value() == 0); // deleted no “x”
	ret = redis.lrem("list", 0, "b");
	assert(ret.type() == REDIS_LONG && ret.value() == 2); // deleted 2 “b”s from the start
	ret = redis.lrem("list", 0, "c");
	assert(ret.type() == REDIS_LONG && ret.value() == 1); // deleted 1 “a”

	redis.del("list");
	redis.set("list", "actually not a list");
	ret = redis.lrem("list", 0, "x");
	assert(ret.type() == REDIS_ERR);
}

void
testLtrim(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "val0");
	redis.lpush("list", "val1");
	redis.lpush("list", "val2");
	redis.lpush("list", "val3");

	redis::Response ret = redis.ltrim("list", 0, 2);
	assert(ret.type() == REDIS_BOOL && ret.boolVal());

	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 3);

	redis.ltrim("list", 0, 0);
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	ret = redis.lpop("list");
	assert(ret.type() == REDIS_STRING && ret.str() == "val3");

	ret = redis.ltrim("list", 10, 10000);
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	ret = redis.ltrim("list", 10000, 10);
	assert(ret.type() == REDIS_BOOL && ret.boolVal());

	// test invalid type
	redis.set("list", "not a list...");
	ret = redis.ltrim("list", 0, 1);
	assert(ret.type() == REDIS_BOOL && ret.boolVal() == false);
}

void
testLset(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "val0");
	redis.lpush("list", "val1");
	redis.lpush("list", "val2");

	redis::Response ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "val1");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");

	ret = redis.lset("list", 1, "valx");
	assert(ret.type() == REDIS_BOOL && ret.boolVal() == true);

	ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.str() == "val2");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.str() == "valx");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.str() == "val0");
}

void
testLrange(redis::Client &redis) {

	redis::List l;

	redis.del("list");
	redis.lpush("list", "val0");
	redis.lpush("list", "val1");
	redis.lpush("list", "val2");

	// pos :   0     1     2
	// pos :  -3    -2    -1
	// list: [val2, val1, val0]

	redis::Response ret = redis.lrange("list", 0, 0);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 1);
	assert(l[0] == redis::Buffer("val2"));

	ret = redis.lrange("list", 0, 1);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 2);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));

	ret = redis.lrange("list", 0, 2);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val0"));

	ret = redis.lrange("list", 0, 3);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val0"));

	ret = redis.lrange("list", 0, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val0"));

	ret = redis.lrange("list", 0, -2);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 2);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));

	ret = redis.lrange("list", -2, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 2);
	assert(l[0] == redis::Buffer("val1"));
	assert(l[1] == redis::Buffer("val0"));

	redis.del("list");
	ret = redis.lrange("list", 0, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 0);
}

void
testRpopLpush(redis::Client &redis) {

	// standard case.
	redis.del("x");
	redis.del("y");
	redis.lpush("x", "abc");
	redis.lpush("x", "def"); // x = [def, abc]
	redis.lpush("y", "123");
	redis.lpush("y", "456"); // x = [456, 123]

	redis::Response ret = redis.rpoplpush("x", "y");
	assert(ret.type() == REDIS_STRING && ret.str() == "abc");	// we RPOP x, yielding abc.

	ret = redis.lrange("x", 0, -1);	// only def remains in x.
	assert(ret.type() == REDIS_LIST);
	redis::List l = ret.array();
	assert(l.size() == 1);
	assert(l[0] == redis::Buffer("def"));

	ret = redis.lrange("y", 0, -1);	// abc has been lpushed to y.
	assert(ret.type() == REDIS_LIST);
	l = ret.array();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("abc"));
	assert(l[1] == redis::Buffer("456"));
	assert(l[2] == redis::Buffer("123"));

	// with an empty source, expecting no change.
	redis.del("x");
	redis.del("y");

	ret = redis.rpoplpush("x", "y");
	assert(ret.type() == REDIS_ERR);

	ret = redis.lrange("x", 0, -1);	// x is still empty
	assert(ret.type() == REDIS_LIST && ret.size() == 0);
	ret = redis.lrange("y", 0, -1);	// y is still empty
	assert(ret.type() == REDIS_LIST && ret.size() == 0);
}

void
testSadd(redis::Client &redis) {

	redis.del("set");
	redis::Response ret = redis.sadd("set", "val");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	ret = redis.sadd("set", "val");
	assert(ret.type() == REDIS_BOOL && !ret.boolVal());

	ret = redis.sismember("set", "val");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
	ret = redis.sismember("set", "val2");
	assert(ret.type() == REDIS_BOOL && !ret.boolVal());

	ret = redis.sadd("set", "val2");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());

	ret = redis.sismember("set", "val2");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());
}

void
testScard(redis::Client &redis) {
        redis.del("set");

	redis::Response ret = redis.sadd("set", "val");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());

	ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	ret = redis.sadd("set", "val2");
	assert(ret.type() == REDIS_BOOL && ret.boolVal());

	ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.value() == 2);

}

void
testSrem(redis::Client &redis) {
	redis.del("set");

	redis.sadd("set", "val0");
	redis.sadd("set", "val1");
	redis.srem("set", "val0");

	redis::Response ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.value() == 1);

	redis.srem("set", "val1");

	ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.value() == 0);
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
	testMGet(r);
//	testExpire(r);
//	testExpireAt(r);
	testSetNX(r);
	testIncr(r);
	testDecr(r);
	testExists(r);
	testKeys(r);
	testDelete(r);
	testType(r);
	testLPushLPop(r);
	testRPushRPop(r);
	testLlen(r);
	testLindex(r);
	testLrem(r);
	testLtrim(r);
	testLset(r);
	testLrange(r);
	testRpopLpush(r);
	testSadd(r);
	testScard(r);
	testSrem(r);


	cout << endl << tests_passed << " tests passed, " << tests_failed << " failed." << endl;

	return 0;
}

