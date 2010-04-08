#include "redis.h"
#include "redisCommand.h"

#include <iostream>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdlib>

using namespace std;

Redis::Redis() :
	m_multi(false),
	m_pipeline(false) {
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
	(void)ret; // TODO: check return value.
}

RedisResponse
Redis::run(RedisCommand &c, ResponseReader fun) {

	if(m_pipeline) { // just enqueue the request
		m_readers.push_back(fun); // remember the reading fun.

		// concat command
		RedisString cmd = c.get();
		m_cmd.insert(m_cmd.end(), cmd.begin(), cmd.end());
		return RedisResponse(REDIS_QUEUED);
	}
	// otherwise, exec
	run(c);

	if(m_multi) { // either queued, read confirmation
		m_readers.push_back(fun);
		return read_queued();
	} else { // or read normal reply.
		return (this->*fun)();
	}
}

void
Redis::discard() {
	m_multi = false;
	m_pipeline = false;
	m_cmd.clear();
	m_readers.clear();
}


bool
Redis::multi() {
	if(m_multi || m_pipeline) {
		return false;
	}
	m_multi = true;
	return true;
}

bool
Redis::pipeline() {
	if(m_multi || m_pipeline) {
		return false;
	}
	m_pipeline = true;
	return true;
}

vector<RedisResponse>
Redis::exec() {
	vector<RedisResponse> ret;

	if(m_multi) {
		RedisCommand cmd("EXEC");
		run(cmd);
	} else if (m_pipeline) {
		int ret = write(m_fd, &m_cmd[0], m_cmd.size());
		(void)ret; // TODO: check return value.
	}

	// read back each response
	vector<ResponseReader>::const_iterator funptr;
	for(funptr = m_readers.begin(); funptr != m_readers.end(); funptr++) {
		RedisResponse resp = (this->**funptr)();
		ret.push_back(resp);
	}

	// cleanup
	discard();

	return ret;
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

RedisResponse
Redis::read_status_code() {
	RedisResponse ret(REDIS_BOOL);

	std::string str = getline();
	if(str[0] == '+') {
		ret.setBool(true);
	}
	return ret;
}
RedisResponse
Redis::read_single_line() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == '+') {
		RedisString s(&str[1]);
		ret.type(REDIS_STRING);
		ret.setString(s);
	}
	return ret;
}

RedisResponse
Redis::read_queued() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str == "+QUEUED") {
		ret.type(REDIS_QUEUED);
	}
	return ret;
}

RedisResponse
Redis::read_integer() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == ':') {
		ret.type(REDIS_LONG);
		ret.setLong(::atol(&str[1]));
	}
	return ret;
}

RedisResponse
Redis::read_double() {
	RedisResponse ret = read_string();
	if(ret.type() != REDIS_STRING) {
		return ret;
	}
	string s = ret.str();
	ret.type(REDIS_DOUBLE);
	ret.setDouble(atof(&s[0]));

	return ret;
}

/**
 * Reads :1 as true, :0 as false.
 */
RedisResponse
Redis::read_integer_as_bool() {
	RedisResponse ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == ':') {
		long l;
		switch((l = ::atol(&str[1]))) {
			case 0:
			case 1:
				ret.type(REDIS_BOOL);
				ret.setBool(l == 1);
				break;
		}
	}
	return ret;
}

RedisResponse
Redis::read_multi_bulk() {
	RedisResponse err(REDIS_ERR);
	RedisResponse ret(REDIS_LIST);

	std::string str = getline();
	if(str[0] != '*') {
		return err;
	}
	long count = ::atol(&str[1]);
	if(count <= 0) {
		return ret;
	}
	for(long i = 0; i < count; ++i) {
		RedisResponse s = read_string();
		if(s.type() != REDIS_STRING) {
			return err;
		}
		ret.addString(s.string());
	}
	return ret;
}

/* actual redis commands */

RedisResponse
Redis::auth(RedisString key) {
	RedisCommand cmd("AUTH");

	cmd << key;
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::select(int index) {
	RedisCommand cmd("SELECT");

	cmd << (long)index;
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::keys(RedisString pattern) {
	RedisCommand cmd("KEYS");

	cmd << pattern;
	return run(cmd, &Redis::read_multi_bulk);
}
RedisResponse
Redis::dbsize(int index) {
	RedisCommand cmd("DBSIZE");

	cmd << (long)index;
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::lastsave() {
	RedisCommand cmd("LASTSAVE");
	return run(cmd, &Redis::read_integer);
}
RedisResponse
Redis::flushdb() {
	RedisCommand cmd("FLUSHDB");
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::flushall() {
	RedisCommand cmd("FLUSHALL");
	return run(cmd, &Redis::read_status_code);
}
RedisResponse
Redis::save() {
	RedisCommand cmd("SAVE");
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::bgsave() {
	RedisCommand cmd("BGSAVE");
	return run(cmd, &Redis::read_status_code);
}


RedisResponse
Redis::get(RedisString key){
	RedisCommand cmd("GET");

	cmd << key;
	return run(cmd, &Redis::read_string);
}

RedisResponse 
Redis::set(RedisString key, RedisString val) {
	RedisCommand cmd("SET");

	cmd << key << val;
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::getset(RedisString key, RedisString val) {
	RedisCommand cmd("GETSET");

	cmd << key << val;
	return run(cmd, &Redis::read_string);
}

RedisResponse
Redis::setNx(RedisString key, RedisString val) {
	RedisCommand cmd("SETNX");
	cmd << key << val;
	return run(cmd, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::exists(RedisString key) {
	RedisCommand cmd("EXISTS");
	cmd << key;
	return run(cmd, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::del(RedisString key) {
	RedisCommand cmd("DEL");
	cmd << key;
	return run(cmd, &Redis::read_integer);
}
RedisResponse
Redis::del(RedisList keys) {
	return generic_multi_parameter("DEL", keys, &Redis::read_integer);
}

RedisResponse
Redis::mget(RedisList keys) {
	return generic_multi_parameter("MGET", keys, &Redis::read_multi_bulk);
}

RedisResponse
Redis::expire(RedisString key, long ttl) {
	return generic_key_int_return_int("EXPIRE", key, ttl);
}
RedisResponse
Redis::expireAt(RedisString key, long timestamp) {
	return generic_key_int_return_int("EXPIREAT", key, timestamp);
}

RedisResponse
Redis::mset(RedisList keys, RedisList vals) {

	return generic_mset("MSET", keys, vals, &Redis::read_status_code);
}

RedisResponse
Redis::msetnx(RedisList keys, RedisList vals) {

	return generic_mset("MSETNX", keys, vals, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::info() {
	RedisCommand cmd("INFO");
	return run(cmd, &Redis::read_info_reply);
}

RedisResponse
Redis::read_info_reply() {

	RedisResponse ret = read_string();

	if(ret.type() != REDIS_STRING) {
		cout << "FAIL" << endl;
		return RedisResponse(REDIS_ERR);
	}

	string s = ret.str();
	ret.type(REDIS_INFO_MAP);

	// now split it on '\r'
	size_t pos = 0, old_pos = 0;
	while(true) {
		pos = s.find('\r', pos);
		if(pos == string::npos) {
			break;
		}
		string line = s.substr(old_pos, pos - old_pos);
		pos += 2;
		old_pos = pos;

		// now split the line on ':'
		size_t colon_pos = line.find(':');
		if(colon_pos != string::npos) {
			string key = line.substr(0, colon_pos);
			string val = line.substr(colon_pos + 1);

			ret.addString(key, val);
		}
	}

	return ret;
}

RedisResponse 
Redis::incr(RedisString key, int val) {

	return generic_key_int_return_int("INCR", key, val, true);
}
RedisResponse 
Redis::decr(RedisString key, int val) {

	return generic_key_int_return_int("DECR", key, val, true);
}
RedisResponse
Redis::rename(RedisString src, RedisString dst) {

	RedisCommand cmd("RENAME");
	cmd << src << dst;
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::renameNx(RedisString src, RedisString dst) {

	RedisCommand cmd("RENAMENX");
	cmd << src << dst;
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::randomKey() {
	RedisCommand cmd("RANDOMKEY");
	return run(cmd, &Redis::read_single_line);
}

RedisResponse
Redis::ttl(RedisString key) {
	RedisCommand cmd("TTL");
	cmd << key;
	return run(cmd, &Redis::read_integer);
}


RedisResponse
Redis::ping() {
	RedisCommand cmd("PING");
	return run(cmd, &Redis::read_status_code);
}

/* List commands */

RedisResponse
Redis::lpush(RedisString key, RedisString val) {
	return generic_push("LPUSH", key, val);
}
RedisResponse
Redis::rpush(RedisString key, RedisString val) {
	return generic_push("RPUSH", key, val);
}

RedisResponse
Redis::rpoplpush(RedisString src, RedisString dst) {

	RedisCommand cmd("RPOPLPUSH");
	cmd << src << dst;
	return run(cmd, &Redis::read_string);
}

RedisResponse
Redis::llen(RedisString key) {
	RedisCommand cmd("LLEN");
	cmd << key;
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::lpop(RedisString key) {
	return generic_pop("LPOP", key);
}
RedisResponse
Redis::rpop(RedisString key) {
	return generic_pop("RPOP", key);
}

RedisResponse
Redis::ltrim(RedisString key, int start, int end) {

	RedisCommand cmd("LTRIM");
	cmd << key << (long)start << (long)end;
	return run(cmd, &Redis::read_status_code);
}

RedisResponse
Redis::lindex(RedisString key, int pos) {

	RedisCommand cmd("LINDEX");
	cmd << key << (long)pos;
	return run(cmd, &Redis::read_string);
}

RedisResponse
Redis::lrem(RedisString key, int count, RedisString val) {

	return generic_list_item_action("LREM", key, count, val, &Redis::read_integer);
}

RedisResponse
Redis::lset(RedisString key, int pos, RedisString val) {

	return generic_list_item_action("LSET", key, pos, val, &Redis::read_status_code);
}

RedisResponse
Redis::lrange(RedisString key, int start, int end) {

	RedisCommand cmd("LRANGE");
	cmd << key << (long)start << (long)end;
	return run(cmd, &Redis::read_multi_bulk);
}

/* Set commands */

RedisResponse
Redis::sadd(RedisString key, RedisString val) {
	return generic_set_key_value("SADD", key, val);
}

RedisResponse
Redis::srem(RedisString key, RedisString val) {
	return generic_set_key_value("SREM", key, val);
}

RedisResponse
Redis::spop(RedisString key) {
	return generic_pop("SPOP", key);
}

RedisResponse
Redis::scard(RedisString key) {
	return generic_card("SCARD", key);
}
RedisResponse
Redis::sismember(RedisString key, RedisString val) {
	return generic_set_key_value("SISMEMBER", key, val);
}

RedisResponse
Redis::srandmember(RedisString key) {
	return generic_pop("SRANDMEMBER", key);
}

RedisResponse
Redis::smove(RedisString src, RedisString dst, RedisString member) {

	RedisCommand cmd("SMOVE");
	cmd << src << dst << member;
	return run(cmd, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::sinter(RedisList keys) {
	return generic_multi_parameter("SINTER", keys, &Redis::read_multi_bulk);
}
RedisResponse
Redis::sunion(RedisList keys) {
	return generic_multi_parameter("SUNION", keys, &Redis::read_multi_bulk);
}
RedisResponse
Redis::sdiff(RedisList keys) {
	return generic_multi_parameter("SDIFF", keys, &Redis::read_multi_bulk);
}
RedisResponse
Redis::sinterstore(RedisList keys) {
	return generic_multi_parameter("SINTERSTORE", keys, &Redis::read_integer);
}
RedisResponse
Redis::sunionstore(RedisList keys) {
	return generic_multi_parameter("SUNIONSTORE", keys, &Redis::read_integer);
}
RedisResponse
Redis::sdiffstore(RedisList keys) {
	return generic_multi_parameter("SDIFFSTORE", keys, &Redis::read_integer);
}

/* zset commands */

RedisResponse
Redis::zadd(RedisString key, double score, RedisString member) {
	RedisCommand cmd("ZADD");

	cmd << key << score << member;
	return run(cmd, &Redis::read_integer_as_bool);
}
RedisResponse
Redis::zrem(RedisString key, RedisString member) {
	RedisCommand cmd("ZREM");

	cmd << key << member;
	return run(cmd, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::zincrby(RedisString key, double score, RedisString member) {
	RedisCommand cmd("ZINCRBY");

	cmd << key << score << member;
	return run(cmd, &Redis::read_double);
}

RedisResponse
Redis::zscore(RedisString key, RedisString member) {
	RedisCommand cmd("ZSCORE");

	cmd << key << member;
	return run(cmd, &Redis::read_double);
}
RedisResponse
Redis::zrank(RedisString key, RedisString member) {
	return generic_zrank("ZRANK", key, member);
}
RedisResponse
Redis::zrevrank(RedisString key, RedisString member) {
	return generic_zrank("ZREVRANK", key, member);
}
RedisResponse
Redis::zrange(RedisString key, long start, long end, bool withscores) {
	return generic_zrange("ZRANGE", key, start, end, withscores);
}
RedisResponse
Redis::zrevrange(RedisString key, long start, long end, bool withscores) {
	return generic_zrange("ZREVRANGE", key, start, end, withscores);
}

RedisResponse
Redis::zcard(RedisString key) {
	return generic_card("ZCARD", key);
}

RedisResponse
Redis::zcount(RedisString key, long start, long end) {
	return generic_z_start_end_int("ZCOUNT", key, start, end);
}
RedisResponse
Redis::zremrangebyrank(RedisString key, long min, long max) {
	return generic_z_start_end_int("ZREMRANGEBYRANK", key, min, max);
}
RedisResponse
Redis::zremrangebyscore(RedisString key, long min, long max) {
	return generic_z_start_end_int("ZREMRANGEBYSCORE", key, min, max);
}

RedisResponse
Redis::zrangebyscore(RedisString key, long min, long max, bool withscores) {

	RedisCommand cmd("ZRANGEBYSCORE");
	cmd << key << min << max;
	if(withscores) {
		cmd << RedisString("WITHSCORES");
	}

	return run(cmd, &Redis::read_multi_bulk);
}

RedisResponse
Redis::zrangebyscore(RedisString key, long min, long max, long start, long end, bool withscores) {

	RedisCommand cmd("ZRANGEBYSCORE");
	cmd << key << min << max << RedisString("LIMIT") << start << end;
	if(withscores) {
		cmd << RedisString("WITHSCORES");
	}

	return run(cmd, &Redis::read_multi_bulk);
}

RedisResponse
Redis::zunion(RedisString key, RedisList keys) {
	vector<double> v;
	return zunion(key, keys, v, "");
}
RedisResponse
Redis::zunion(RedisString key, RedisList keys, string aggregate) {
	vector<double> v;
	return zunion(key, keys, v, aggregate);
}
RedisResponse
Redis::zunion(RedisString key, RedisList keys, vector<double> weights) {
	return zunion(key, keys, weights, "");
}
RedisResponse
Redis::zunion(RedisString key, RedisList keys, vector<double> weights, string aggregate) {
	return generic_z_set_operation("ZUNION", key, keys, weights, aggregate);
}

RedisResponse
Redis::zinter(RedisString key, RedisList keys) {
	vector<double> v;
	return zunion(key, keys, v, "");
}
RedisResponse
Redis::zinter(RedisString key, RedisList keys, string aggregate) {
	vector<double> v;
	return zunion(key, keys, v, aggregate);
}
RedisResponse
Redis::zinter(RedisString key, RedisList keys, vector<double> weights) {
	return zunion(key, keys, weights, "");
}
RedisResponse
Redis::zinter(RedisString key, RedisList keys, vector<double> weights, string aggregate) {
	return generic_z_set_operation("ZINTER", key, keys, weights, aggregate);
}

RedisResponse
Redis::generic_z_set_operation(string keyword, RedisString key, RedisList keys,
		vector<double> weights, string aggregate) {

	if(weights.size() != 0 && keys.size() != weights.size()) {
		return RedisResponse(REDIS_ERR);
	}
	RedisCommand cmd(keyword);
	cmd << key << (long)keys.size();

	RedisList::const_iterator i_key;
	for(i_key = keys.begin(); i_key != keys.end(); i_key++) {
		cmd << *i_key;
	}
	if(weights.size()) {

		cmd << RedisString("WEIGHTS");

		vector<double>::const_iterator w_key;
		for(w_key = weights.begin(); w_key != weights.end(); w_key++) {
			cmd << *w_key;
		}
	}

	if(aggregate.size()) {
		string s = "AGGREGATE " + aggregate;
		cmd << RedisString(s.c_str());
	}

	return run(cmd, &Redis::read_integer);
}

/* hash commands */

RedisResponse
Redis::hset(RedisString key, RedisString field, RedisString val) {
	RedisCommand cmd("HSET");
	cmd << key << field << val;
	return run(cmd, &Redis::read_integer_as_bool);
}
RedisResponse
Redis::hget(RedisString key, RedisString field) {
	RedisCommand cmd("HSET");
	cmd << key << field;
	return run(cmd, &Redis::read_string);
}

RedisResponse
Redis::hdel(RedisString key, RedisString field) {
	RedisCommand cmd("HDEL");
	cmd << key << field;
	return run(cmd, &Redis::read_integer_as_bool);
}
RedisResponse
Redis::hexists(RedisString key, RedisString field) {
	RedisCommand cmd("HEXISTS");
	cmd << key << field;
	return run(cmd, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::hlen(RedisString key) {
	RedisCommand cmd("HLEN");
	cmd << key;
	return run(cmd, &Redis::read_integer);
}
RedisResponse
Redis::hkeys(RedisString key) {
	return generic_h_simple_list("HKEYS", key);
}

RedisResponse
Redis::hvals(RedisString key) {
	return generic_h_simple_list("HVALS", key);
}

RedisResponse
Redis::hgetall(RedisString key) {
	return generic_h_simple_list("HGETALL", key);
}

RedisResponse
Redis::hincrby(RedisString key, RedisString field, double d) {
	RedisCommand cmd("HINCRBY");
	cmd << key << field << d;
	return run(cmd, &Redis::read_double);
}

/* generic commands below */

RedisResponse
Redis::generic_z_start_end_int(string keyword, RedisString key, long start, long end) {

	RedisCommand cmd(keyword);
	cmd << key << start << end;
	return run(cmd, &Redis::read_integer);
}


RedisResponse
Redis::generic_zrange(string keyword, RedisString key, long start, long end, bool withscores) {

	RedisCommand cmd(keyword);

	cmd << key << start << end;
	if(withscores) {
		cmd << RedisString("WITHSCORES");
	}
	return run(cmd, &Redis::read_multi_bulk);
}

RedisResponse
Redis::generic_zrank(string keyword, RedisString key, RedisString member) {
	RedisCommand cmd(keyword);
	cmd << key << member;
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::generic_multi_parameter(string keyword, RedisList &keys, ResponseReader fun) {
	RedisCommand cmd(keyword);
	RedisList::const_iterator key;
	for(key = keys.begin(); key != keys.end(); key++) {
		cmd << *key;
	}
	return run(cmd, fun);
}

RedisResponse
Redis::generic_pop(string keyword, RedisString key){
	RedisCommand cmd(keyword);

	cmd << key;
	return run(cmd, &Redis::read_string);
}

RedisResponse
Redis::generic_push(string keyword, RedisString key, RedisString val) {

	RedisCommand cmd(keyword);
	cmd << key << val;
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::generic_key_int_return_int(std::string keyword, RedisString key, int val, bool addBy) {

	if(val > 1 && addBy) {
		keyword = keyword + "BY";
	}
	RedisCommand cmd(keyword);
	cmd << key;
	if(!addBy || (val > 1 && addBy)) {
		cmd << (long)val;
	}
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::generic_list_item_action(string keyword, RedisString key, int n,
		RedisString val, ResponseReader fun) {
	RedisCommand cmd(keyword);
	cmd << key << (long)n << val;

	return run(cmd, fun);
}

RedisResponse
Redis::generic_set_key_value(string keyword, RedisString key, RedisString val) {

	RedisCommand cmd(keyword);
	cmd << key << val;
	return run(cmd, &Redis::read_integer_as_bool);
}

RedisResponse
Redis::generic_card(string keyword, RedisString key) {

	RedisCommand cmd(keyword);
	cmd << key;
	return run(cmd, &Redis::read_integer);
}

RedisResponse
Redis::generic_mset(string keyword, RedisList keys, RedisList vals, ResponseReader fun) {

	if(keys.size() != vals.size() || keys.size() == 0) {
		return RedisResponse(REDIS_ERR);
	}

	RedisCommand cmd(keyword);
	RedisList::const_iterator key;
	RedisList::const_iterator val;
	for(key = keys.begin(), val = vals.begin(); key != keys.end(); key++, val++) {
		cmd << *key << *val;
	}

	return run(cmd, fun);
}

RedisResponse
Redis::generic_h_simple_list(string keyword, RedisString key) {
	RedisCommand cmd(keyword);
	cmd << key;

	return run(cmd, &Redis::read_multi_bulk);
}

