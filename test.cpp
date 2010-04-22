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
	assert(r.get<bool>() == true);
}
void
test1000(redis::Client &redis, int count) {
	char *buf = new char[count+1];
	buf[count] = 0;
	fill(buf, buf + count, 'A');

	redis::Response rSet = redis.set("x", redis::Buffer(buf, count));
	assert(rSet.type() == REDIS_BOOL && rSet.get<bool>() == true);

	redis::Response rGet = redis.get("x");
	assert(rGet.type() == REDIS_STRING && rGet.get<string>() == string(buf, count));

	delete[] buf;
}

void
testErr(redis::Client &redis) {

	redis.set("x", "-ERR");
	redis::Response rGet = redis.get("x");
	assert(rGet.type() == REDIS_STRING && rGet.get<string>() == "-ERR");
}

void
testSet(redis::Client &redis) {
	redis::Response ret(REDIS_ERR);

	// set value to "nil"
	ret = redis.set("x", "nil");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "nil");

	// get twice
	ret = redis.set("x", "val");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val");
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val");

	// set, delete, and get → not found.
	ret = redis.set("x", "val");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	redis.del("x");
	ret = redis.get("x");
	assert(ret.type() == REDIS_ERR);

	// binary data
	redis::Buffer val("v\0a\0l", 5);
	ret = redis.set("x", val);
	ret = redis.get("x");
	assert(ret.type() == REDIS_STRING && ret.get<redis::Buffer>() == val);
	
	// binary key
	redis::Buffer k("v\0a\0l", 5);
	ret = redis.set(k, "ok");
	ret = redis.get(k);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "ok");
	
	// binary key and value
	ret = redis.set(k, val);
	ret = redis.get(k);
	assert(ret.type() == REDIS_STRING && ret.get<redis::Buffer>() == val);

	string keyRN = "abc\r\ndef";
	string valRN = "val\r\nxyz";
	ret = redis.set(keyRN, valRN);
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.get(keyRN);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == valRN);
}

void
testGetSet(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.getset("key", "42");
	assert(ret.type() == REDIS_ERR);

	ret = redis.getset("key", "123");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "42");

	ret = redis.getset("key", "123");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "123");
}

void
testRandomKey(redis::Client &redis) {

	bool success_key_string = true, success_types = true, success_values = true;

	for(int i = 0; i < 1000; ++i) {
		redis::Response ret = redis.randomkey();
		success_key_string &= (ret.type() == REDIS_STRING);

		ret = redis.exists(ret.get<redis::Buffer>());
		success_types &= (ret.type() == REDIS_BOOL);
		success_values &= (ret.get<bool>());
	}
	assert(success_key_string);
	assert(success_types);
	assert(success_values);
}

void
testRename(redis::Client &redis) {

	// strings
	redis.del("key0");
	redis.set("key0", "val0");
	redis.rename("key0", "key1");
	assert(redis.get("key0").type() == REDIS_ERR);
	assert(redis.get("key1").get<string>() == "val0");

	// lists
	redis.del("key0");
	redis.lpush("key0", "val0");
	redis.lpush("key0", "val1");
	redis.rename("key0", "key1");
	assert(redis.lrange("key0", 0, -1).size() == 0);

	redis::Response ret = redis.lrange("key1", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.get<vector<redis::Buffer> >()[0][0] == string("val1"));
	assert(&ret.get<vector<redis::Buffer> >()[1][0] == string("val0"));
}

void
testRenameNx(redis::Client &redis) {

	// strings
	redis.del("key0");
	redis.set("key0", "val0");
	redis.set("key1", "val1");
	redis::Response ret = redis.renamenx("key0", "key1");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>() == false);
	ret = redis.get("key0");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");
	ret = redis.get("key1");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val1");

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
	assert(&ret.get<vector<redis::Buffer> >()[0][0] == string("val1"));
	assert(&ret.get<vector<redis::Buffer> >()[1][0] == string("val0"));

	ret = redis.lrange("key1", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.get<vector<redis::Buffer> >()[0][0] == string("val1-1"));
	assert(&ret.get<vector<redis::Buffer> >()[1][0] == string("val1-0"));

	redis.del("key2");
	redis.renamenx("key0", "key2");
	ret = redis.lrange("key0", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 0);

	ret = redis.lrange("key2", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	assert(&ret.get<vector<redis::Buffer> >()[0][0] == string("val1"));
	assert(&ret.get<vector<redis::Buffer> >()[1][0] == string("val0"));
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
	assert(ret.size() == 1);
	assert(ret.get("k1") == redis::Buffer("v1"));

	// all good
	l.clear();
	l.push_back(redis::Buffer("k1"));
	l.push_back(redis::Buffer("k2"));
	l.push_back(redis::Buffer("k3"));
	ret = redis.mget(l);
	assert(ret.size() == 3);

	assert(ret.get("k1") == redis::Buffer("v1"));
	assert(ret.get("k2") == redis::Buffer("v2"));
	assert(ret.get("k3") == redis::Buffer("v3"));

	// invalid key.
	l.clear();
	l.push_back(redis::Buffer("k1"));
	l.push_back(redis::Buffer("k3"));
	l.push_back(redis::Buffer("NoKey"));
	ret = redis.mget(l);
	assert(ret.size() == 2);
	assert(ret.get("k1") == redis::Buffer("v1"));
	assert(ret.get("k3") == redis::Buffer("v3"));

	try {
		ret.get("NoKey");
		assert(false);
	} catch(...) {
		assert(true);
	}
}


void
testExpire(redis::Client &redis) {

	redis.del("key");
	redis.set("key", "value");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "value");

	redis.expire("key", 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "value");
	sleep(2);

	ret = redis.get("key");
	assert(ret.type() == REDIS_ERR);
}

void
testExpireAt(redis::Client &redis) {

	redis.del("key");
	redis.set("key", "value");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "value");

	time_t now = time(0);
	redis.expireat("key", now + 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "value");
	sleep(2);

	ret = redis.get("key");
	assert(ret.type() == REDIS_ERR);
}

void
testSetNX(redis::Client &redis) {

	redis.set("key", "42");
	redis::Response ret = redis.setnx("key", "err");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "42");

	redis.del("key");
	ret = redis.setnx("key", "42");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "42");
}

void
testIncr(redis::Client &redis) {

	redis.set("key", "0");
	redis.incr("key");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "1");
	redis.incr("key");

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "2");

	redis.incr("key", 3);
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "5");

	// increment a non-numeric string
	redis.del("key");
	redis.set("key", "abc");
	ret = redis.incr("key");
	assert(ret.type() == REDIS_ERR);
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "abc");
}

void
testDecr(redis::Client &redis) {

	redis.set("key", "5");
	redis.decr("key");

	redis::Response ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "4");
	redis.decr("key");

	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "3");

	redis.decr("key", 2);
	ret = redis.get("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "1");
}

void
testExists(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.exists("key");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>() == false);

	redis.set("key", "value");
	ret = redis.exists("key");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>() == true);
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
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	// multiple, all existing
	redis.set("x", "0");
	redis.set("y", "1");
	redis.set("z", "2");
	redis::List l;
	l.push_back("x");
	l.push_back("y");
	l.push_back("z");
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 3);

	// multiple, none existing
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);
	
	// multiple, some existing
	redis.set("y", "1");
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	redis.set("x", "0");
	redis.set("y", "1");
	ret = redis.del(l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);
}

void
testType(redis::Client &redis) {

	redis.set("key", "val");
	redis::Response ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == redis::Client::STRING);

	redis.del("key");
	redis.lpush("key", "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == redis::Client::LIST);

	redis.del("key");
	redis.sadd("key", "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == redis::Client::SET);

	redis.del("key");
	redis.zadd("key", 42, "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == redis::Client::ZSET);

	redis.del("key");
	redis.hset("key", "field", "val");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == redis::Client::HASH);

	redis.del("key");
	ret = redis.type("key");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == redis::Client::NONE);
}

void
testLPushLPop(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.lpush("key", "val0");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);
	ret = redis.lpush("key", "val1");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);
	ret = redis.lpush("key", "val2");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 3);

	ret = redis.lpop("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");
	ret = redis.lpop("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val1");
	ret = redis.lpop("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");
	ret = redis.lpop("key");
	assert(ret.type() == REDIS_ERR);

	// binary push
	ret = redis.lpush("key", redis::Buffer("v\0a\0l\0", 6));
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);
	ret = redis.lpop("key");
	string s = ret.get<string>();
	assert(ret.type() == REDIS_STRING && ::memcmp(s.c_str(),"v\0a\0l\0", 6) == 0);
}

void
testRPushRPop(redis::Client &redis) {

	redis.del("key");
	redis::Response ret = redis.rpush("key", "val0");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);
	ret = redis.rpush("key", "val1");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);
	ret = redis.rpush("key", "val2");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 3);

	ret = redis.rpop("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");
	ret = redis.rpop("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val1");
	ret = redis.rpop("key");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");
	ret = redis.rpop("key");
	assert(ret.type() == REDIS_ERR);

	// binary push
	ret = redis.rpush("key", redis::Buffer("v\0a\0l\0", 6));
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);
	ret = redis.rpop("key");
	string s = ret.get<string>();
	assert(ret.type() == REDIS_STRING && ::memcmp(s.c_str(),"v\0a\0l\0", 6) == 0);
}

void
testLlen(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "val");

	redis::Response ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	redis.lpush("list", "val2");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);

	ret = redis.lpop("list");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	ret = redis.lpop("list");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0); // empty list is 0

	redis.del("list");
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0); // non-existent list is 0

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
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");

	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val1");

	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");

	ret = redis.lindex("list", 3);
	assert(ret.type() == REDIS_ERR);

	ret = redis.lindex("list", -1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");

	ret = redis.lindex("list", -2);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val1");

	ret = redis.lindex("list", -3);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");

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
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2); // deleted 2 “b”s from the start
	ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "c");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "c");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "c");
	ret = redis.lindex("list", 3);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "a");

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
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2); // deleted 2 “c”s from the end.
	ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "c");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "b");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "b");
	ret = redis.lindex("list", 3);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "a");

	// remove each element
	ret = redis.lrem("list", 0, "a");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1); // deleted 1 “a”
	ret = redis.lrem("list", 0, "x");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0); // deleted no “x”
	ret = redis.lrem("list", 0, "b");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2); // deleted 2 “b”s from the start
	ret = redis.lrem("list", 0, "c");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1); // deleted 1 “a”

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
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 3);

	redis.ltrim("list", 0, 0);
	ret = redis.llen("list");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	ret = redis.lpop("list");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val3");

	ret = redis.ltrim("list", 10, 10000);
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.ltrim("list", 10000, 10);
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	// test invalid type
	redis.set("list", "not a list...");
	ret = redis.ltrim("list", 0, 1);
	assert(ret.type() == REDIS_BOOL && ret.get<bool>() == false);
}

void
testLset(redis::Client &redis) {

	redis.del("list");
	redis.lpush("list", "val0");
	redis.lpush("list", "val1");
	redis.lpush("list", "val2");

	redis::Response ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val1");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");

	ret = redis.lset("list", 1, "valx");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>() == true);

	ret = redis.lindex("list", 0);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val2");
	ret = redis.lindex("list", 1);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "valx");
	ret = redis.lindex("list", 2);
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "val0");
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
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 1);
	assert(l[0] == redis::Buffer("val2"));

	ret = redis.lrange("list", 0, 1);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 2);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));

	ret = redis.lrange("list", 0, 2);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val0"));

	ret = redis.lrange("list", 0, 3);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val0"));

	ret = redis.lrange("list", 0, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 3);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val0"));

	ret = redis.lrange("list", 0, -2);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 2);
	assert(l[0] == redis::Buffer("val2"));
	assert(l[1] == redis::Buffer("val1"));

	ret = redis.lrange("list", -2, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 2);
	assert(l[0] == redis::Buffer("val1"));
	assert(l[1] == redis::Buffer("val0"));

	redis.del("list");
	ret = redis.lrange("list", 0, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
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
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "abc");	// we RPOP x, yielding abc.

	ret = redis.lrange("x", 0, -1);	// only def remains in x.
	assert(ret.type() == REDIS_LIST);
	redis::List l = ret.get<vector<redis::Buffer> >();
	assert(l.size() == 1);
	assert(l[0] == redis::Buffer("def"));

	ret = redis.lrange("y", 0, -1);	// abc has been lpushed to y.
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
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
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.sadd("set", "val");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	ret = redis.sismember("set", "val");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.sismember("set", "val2");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	ret = redis.sadd("set", "val2");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.sismember("set", "val2");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
}

void
testScard(redis::Client &redis) {
        redis.del("set");

	redis::Response ret = redis.sadd("set", "val");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	ret = redis.sadd("set", "val2");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);

}

void
testSrem(redis::Client &redis) {
	redis.del("set");

	redis.sadd("set", "val0");
	redis.sadd("set", "val1");
	redis.srem("set", "val0");

	redis::Response ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	redis.srem("set", "val1");

	ret = redis.scard("set");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);
}

void
testSmove(redis::Client &redis) {

	redis.del("set0");
	redis.del("set1");

	redis.sadd("set0", "val0");
	redis.sadd("set0", "val1");

	redis::Response ret = redis.smove("set0", "set1", "val0");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.smove("set0", "set1", "val0");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	ret = redis.smove("set0", "set1", "val-what");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	ret = redis.scard("set0");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);
	ret = redis.scard("set1");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	redis::List l;
	ret = redis.smembers("set0");
	assert(ret.type() == REDIS_LIST && ret.size() == 1);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val1"));

	ret = redis.smembers("set1");
	assert(ret.type() == REDIS_LIST && ret.size() == 1);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
}

void
testSpop(redis::Client &redis) {

	redis.del("set0");
	redis::Response ret = redis.spop("set0");
	assert(ret.type() == REDIS_ERR);

	redis.sadd("set0", "val0");
	redis.sadd("set0", "val1");

	ret = redis.spop("set0");
	string v0 = ret.get<string>();
	assert(ret.type() == REDIS_STRING && (v0 == "val0" || v0 == "val1"));

	ret = redis.spop("set0");
	string v1 = ret.get<string>();
	assert(ret.type() == REDIS_STRING && (v1 == "val0" || v1 == "val1") && v1 != v0);

	ret = redis.spop("set0");
	assert(ret.type() == REDIS_ERR);
}

void
testSismember(redis::Client &redis) {
	redis.del("set");
	redis.sadd("set", "val0");

	redis::Response ret = redis.sismember("set", "val0");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.sismember("set", "val1");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	redis.srem("set", "val0");

	ret = redis.sismember("set", "val0");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());
}

void
testSmembers(redis::Client &redis) {
	redis.del("set");

	redis.sadd("set", "val0");
	redis.sadd("set", "val1");
	redis.sadd("set", "val2");

	redis::List l;
	l.push_back("val0");
	l.push_back("val1");
	l.push_back("val2");

	redis::List lRedis;
	redis::Response ret = redis.smembers("set");

	assert(ret.type() == REDIS_LIST);
	lRedis = ret.get<vector<redis::Buffer> >();

	int count = 0;
	redis::List::const_iterator i, j;
	for(i = l.begin(); i != l.end(); i++){
		for(j = lRedis.begin(); j != lRedis.end(); j++){

			if(*i == *j) {
				count++;
				break;
			}
		}
	}
	assert(count == 3);
}

void
testSave(redis::Client &redis) {
	redis::Response ret = redis.save();
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());// don't really know how else to test this...
}

void
testBgsave(redis::Client &redis) {
	redis::Response ret = redis.bgsave();
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.bgsave();
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());// the second one should fail.
}

void
testZstuff(redis::Client &redis) {

	vector<double> weights;
	redis::List keys, l;

	redis.del("key with spaces");
	redis::Response ret = redis.zrange("key with spaces", 0, -1);

	assert(ret.type() == REDIS_LIST && ret.size() == 0);

	ret = redis.zadd("key with spaces", 0, "val0");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zadd("key with spaces", 2, "val2");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zadd("key with spaces", 1, "val1");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zadd("key with spaces", 3, "val3");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zrange("key with spaces", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 4);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val2"));
	assert(l[3] == redis::Buffer("val3"));

	ret = redis.zrem("key with spaces", "valX");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	ret = redis.zrem("key with spaces", "val3");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zrange("key with spaces", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 3);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val2"));

	// zGetReverseRange
	ret = redis.zadd("key with spaces", 3, "val3");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zadd("key with spaces", 3, "aal3");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.zrangebyscore("key with spaces", 0, 3);
	assert(ret.type() == REDIS_LIST && ret.size() == 5);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val2"));
	assert(l[3] == redis::Buffer("val3") || l[3] == redis::Buffer("aal3"));
	assert(l[4] == redis::Buffer("val3") || l[4] == redis::Buffer("aal3"));

	// withscores

	redis.zrem("key with spaces", "aal3");

	ret = redis.zrangebyscore("key with spaces", 0, 3, true);
	assert(ret.type() == REDIS_LIST && ret.size() == 8);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("0"));
	assert(l[2] == redis::Buffer("val1"));
	assert(l[3] == redis::Buffer("1"));
	assert(l[4] == redis::Buffer("val2"));
	assert(l[5] == redis::Buffer("2"));
	assert(l[6] == redis::Buffer("val3"));
	assert(l[7] == redis::Buffer("3"));

	// limit

	ret = redis.zrangebyscore("key with spaces", 0, 3, 0, 1); // range 0, 3; limit 0, 1.
	assert(ret.type() == REDIS_LIST && ret.size() == 1);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));

	ret = redis.zrangebyscore("key with spaces", 0, 3, 0, 2); // range 0, 3; limit 0, 2.
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));

	ret = redis.zrangebyscore("key with spaces", 0, 3, 1, 2); // range 0, 3; limit 1, 2.
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val1"));
	assert(l[1] == redis::Buffer("val2"));

	ret = redis.zrangebyscore("key with spaces", 0, 1, 0, 100); // range 0, 1; limit 0, 100.
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));

	ret = redis.zcard("key with spaces");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 4);

	ret = redis.zscore("key with spaces", "val1");
	assert(ret.type() == REDIS_DOUBLE && ret.get<double>() == 1.0);

	ret = redis.zscore("key with spaces", "val");
	assert(ret.type() == REDIS_ERR);
	// zincrby
	redis.del("key with spaces");
	ret = redis.zincrby("key with spaces", 1.0, "val1");
	assert(ret.type() == REDIS_DOUBLE && ret.get<double>() == 1.0);
	ret = redis.zscore("key with spaces", "val1");
	assert(ret.type() == REDIS_DOUBLE && ret.get<double>() == 1.0);

	ret = redis.zincrby("key with spaces", 1.5, "val1");
	assert(ret.type() == REDIS_DOUBLE && ret.get<double>() == 2.5);
	ret = redis.zscore("key with spaces", "val1");
	assert(ret.type() == REDIS_DOUBLE && ret.get<double>() == 2.5);

	//zUnion
	redis.del("key1");
	redis.del("key2");
	redis.del("key3");
	redis.del("keyU");

	redis.zadd("key1", 0, "val0");
	redis.zadd("key1", 1, "val1");

	redis.zadd("key2", 2, "val2");
	redis.zadd("key2", 3, "val3");

	redis.zadd("key3", 4, "val4");
	redis.zadd("key3", 5, "val5");

	l.clear();
	l.push_back("key1");
	l.push_back("key3");
	ret = redis.zunion("keyU", l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 4);

	ret = redis.zrange("keyU", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 4);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val4"));
	assert(l[3] == redis::Buffer("val5"));

	// Union on non existing keys
	redis.del("X");
	redis.del("Y");
	redis.del("keyU");

	l.clear();
	l.push_back("X");
	l.push_back("Y");
	ret = redis.zunion("keyU", l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);

	ret = redis.zrange("keyU", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 0);

	// !Exist U Exist
	redis.del("keyU");
	l.clear();
	l.push_back("key1");
	l.push_back("X");
	ret = redis.zunion("keyU", l);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);

	ret = redis.zrange("keyU", 0, -1);
	assert(ret.type() == REDIS_LIST);
	l = ret.get<vector<redis::Buffer> >();
	ret = redis.zrange("key1", 0, -1);
	assert(ret.type() == REDIS_LIST);
	redis::List l1 = ret.get<vector<redis::Buffer> >();
	assert(l == l1);

	// test weighted zUnion
	redis.del("keyZ");
	keys.push_back("key1");
	keys.push_back("key2");
	weights.push_back(1);
	weights.push_back(1);

	ret = redis.zunion("keyZ", keys, weights);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 4);
	ret = redis.zrange("keyZ", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 4);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val1"));
	assert(l[2] == redis::Buffer("val2"));
	assert(l[3] == redis::Buffer("val3"));

	redis.zremrangebyscore("keyZ", 0, 10);
	weights.clear();
	weights.push_back(5);
	weights.push_back(1);
	ret = redis.zunion("keyZ", keys, weights);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 4);
	ret = redis.zrange("keyZ", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 4);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val0"));
	assert(l[1] == redis::Buffer("val2"));
	assert(l[2] == redis::Buffer("val3"));
	assert(l[3] == redis::Buffer("val1"));

	// zInter
	redis.del("key1");
	redis.del("key2");
	redis.del("key3");

	redis.zadd("key1", 0, "val0");
	redis.zadd("key1", 1, "val1");
	redis.zadd("key1", 3, "val3");

	redis.zadd("key2", 2, "val1");
	redis.zadd("key2", 3, "val3");

	redis.zadd("key3", 4, "val3");
	redis.zadd("key3", 5, "val5");

	redis.del("keyI");
	keys.clear();
	keys.push_back("key1");
	keys.push_back("key2");
	ret = redis.zinter("keyI", keys);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);

	ret = redis.zrange("keyI", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val1"));
	assert(l[1] == redis::Buffer("val3"));

	// Inter on non-existing keys
	keys.clear();
	keys.push_back("X");
	keys.push_back("Y");
	ret = redis.zinter("keyX", keys);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);
	ret = redis.zrange("keyX", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 0);

	// !Exist U Exist
	keys.clear();
	keys.push_back("key1");
	keys.push_back("Y");
	ret = redis.zinter("keyY", keys);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);
	ret = redis.zrange("keyY", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 0);

	// test weighted zInter
	redis.del("key1");
	redis.del("key2");
	redis.del("key3");

	redis.zadd("key1", 0, "val0");
	redis.zadd("key1", 1, "val1");
	redis.zadd("key1", 3, "val3");

	redis.zadd("key2", 2, "val1");
	redis.zadd("key2", 1, "val3");

	redis.zadd("key3", 7, "val1");
	redis.zadd("key3", 3, "val3");

	redis.del("keyI");
	keys.clear();
	keys.push_back("key1");
	keys.push_back("key2");
	weights.clear();
	weights.push_back(1);
	weights.push_back(1);
	ret = redis.zinter("keyI", keys, weights);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);

	ret = redis.zrange("keyI", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val1"));
	assert(l[1] == redis::Buffer("val3"));

	// aggregate: MIN
	redis.del("keyI");
	keys.clear();
	keys.push_back("key1");
	keys.push_back("key2");
	keys.push_back("key3");
	weights.clear();
	weights.push_back(1);
	weights.push_back(5);
	weights.push_back(1);
	ret = redis.zinter("keyI", keys, weights, "MIN");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);
	ret = redis.zrange("keyI", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val1"));
	assert(l[1] == redis::Buffer("val3"));

	// aggregate: MAX
	redis.del("keyI");
	ret = redis.zinter("keyI", keys, weights, "MAX");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);
	ret = redis.zrange("keyI", 0, -1);
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("val3"));
	assert(l[1] == redis::Buffer("val1"));
}

void
testHstuff(redis::Client &redis) {

	redis.del("h");
	redis.del("key with spaces");

	// hlen & hset
	redis::Response ret = redis.hlen("h");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);

	ret = redis.hset("h", "a", "a-value");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.hlen("h");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);

	ret = redis.hset("h", "b", "b-value");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.hlen("h");
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);

	// hget
	ret = redis.hget("h", "a");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "a-value");
	ret = redis.hget("h", "b");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "b-value");
	ret = redis.hget("h", "c");
	assert(ret.type() == REDIS_ERR);

	ret = redis.hset("h", "a", "another-value"); // replacement
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());
	ret = redis.hget("h", "a");
	assert(ret.type() == REDIS_STRING && ret.get<string>() == "another-value"); // get the new value

	ret = redis.hget("key with spaces", "c"); // unknown key
	assert(ret.type() == REDIS_ERR);

	// hDel
	ret = redis.hdel("h", "a");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>()); // true on success
	ret = redis.hdel("h", "a");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>()); // false on failure

	redis.del("h");
	redis.hset("h", "x", "a");
	redis.hset("h", "y", "b");

	// hkeys
	ret = redis.hkeys("h");
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	redis::List l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("x") || l[0] == redis::Buffer("y"));
	assert(l[1] == redis::Buffer("x") || l[1] == redis::Buffer("y"));
	assert(l[0] != l[1]);

	// hvals
	ret = redis.hvals("h");
	assert(ret.type() == REDIS_LIST && ret.size() == 2);
	l = ret.get<vector<redis::Buffer> >();
	assert(l[0] == redis::Buffer("a") || l[0] == redis::Buffer("b"));
	assert(l[1] == redis::Buffer("a") || l[1] == redis::Buffer("b"));
	assert(l[0] != l[1]);

	// hgetall
	ret = redis.hgetall("h");
	assert(ret.type() == REDIS_HASH && ret.size() == 2);
	redis::RedisMap m = ret.get<redis::RedisMap>();
	assert(m[redis::Buffer("x")] == redis::Buffer("a"));
	assert(m[redis::Buffer("y")] == redis::Buffer("b"));

	// hExists
	ret = redis.hexists("h", "x");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.hexists("h", "y");
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());
	ret = redis.hexists("h", "w");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());
	redis.del("h");
	ret = redis.hexists("h", "x");
	assert(ret.type() == REDIS_BOOL && !ret.get<bool>());

	// hIncrBy
	redis.del("h");
	ret = redis.hincrby("h", "x", 2);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 2);
	ret = redis.hincrby("h", "x", 1);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 3);

	// non-numeric: atol() and then increment.
	redis.hset("h", "y", "NaN");
	ret = redis.hincrby("h", "y", 1);
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 1);
}

void
testTtl(redis::Client &redis) {

	redis.set("x", "a");
	redis.expire("x", 3);

	for(int i = 3; i > 0; --i) {
		redis::Response ret = redis.ttl("x");
		assert(ret.type() == REDIS_LONG && ret.get<long>() == i);
		sleep(1);
	}
	sleep(1);
	redis::Response ret = redis.get("x");
	assert(ret.type() == REDIS_ERR);
}

void
testFlushdb(redis::Client &redis) {

	redis::Response ret = redis.flushdb();
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.dbsize();
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);
}

void
testFlushall(redis::Client &redis) {

	redis::Response ret = redis.flushall();
	assert(ret.type() == REDIS_BOOL && ret.get<bool>());

	ret = redis.dbsize();
	assert(ret.type() == REDIS_LONG && ret.get<long>() == 0);
}

void
testLastsave(redis::Client &redis) {

	redis.save();
	sleep(2);
	redis::Response ret = redis.lastsave();
	assert(ret.type() == REDIS_LONG && time(0) - ret.get<long>() < 10);
}

void
testMultiExec(redis::Client &redis) {

	redis.multi();
	vector<redis::Response> vret = redis.exec();
	assert(vret.size() == 0);

	redis.multi();
		redis.set("x", "abc");
		redis.set("y", "def");
		redis.get("x");
		redis.get("y");
	vret = redis.exec();

	assert(vret.size() == 4);
	assert(vret[0].type() == REDIS_BOOL && vret[0].get<bool>());
	assert(vret[1].type() == REDIS_BOOL && vret[1].get<bool>());
	assert(vret[2].type() == REDIS_STRING && vret[2].get<string>() == "abc");
	assert(vret[3].type() == REDIS_STRING && vret[3].get<string>() == "def");
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
	testSmove(r);
	testSpop(r);
	testSismember(r);
	testSmembers(r);
//	testSave(r);
//	testBgsave(r);
	testZstuff(r);
	testHstuff(r);
//	testTtl(r);
//	testFlushdb(r); // WARNING, THIS WILL DESTROY ALL YOUR DATA.
//	testFlushall(r); // WARNING, THIS WILL DESTROY ALL YOUR DATA.
//	testLastsave(r);

	testMultiExec(r);


	cout << endl << tests_passed << " tests passed, " << tests_failed << " failed." << endl;

	return 0;
}

