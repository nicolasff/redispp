#include "redis.h"

#include <iostream>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdlib>

using namespace std;

namespace redis {
Client::Client() :
	m_multi(false),
	m_pipeline(false) {
}

bool 
Client::connect(std::string host, short port) {
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
Client::run(Command &c) {

	Buffer cmd = c.get();

	int ret = write(m_fd, &cmd[0], cmd.size());
	(void)ret; // TODO: check return value.
}

Response
Client::run(Command &c, ResponseReader fun) {

	if(m_pipeline) { // just enqueue the request
		m_readers.push_back(fun); // remember the reading fun.

		// concat command
		Buffer cmd = c.get();
		m_cmd.insert(m_cmd.end(), cmd.begin(), cmd.end());
		return Response(REDIS_QUEUED);
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
Client::discard() {
	m_multi = false;
	m_pipeline = false;
	m_cmd.clear();
	m_readers.clear();
}


bool
Client::multi() {
	if(m_multi || m_pipeline) {
		return false;
	}
	m_multi = true;
	return true;
}

bool
Client::pipeline() {
	if(m_multi || m_pipeline) {
		return false;
	}
	m_pipeline = true;
	return true;
}

vector<Response>
Client::exec() {
	vector<Response> ret;

	if(m_multi) {
		Command cmd("EXEC");
		run(cmd);
	} else if (m_pipeline) {
		int ret = write(m_fd, &m_cmd[0], m_cmd.size());
		(void)ret; // TODO: check return value.
	}

	// read back each response
	vector<ResponseReader>::const_iterator funptr;
	for(funptr = m_readers.begin(); funptr != m_readers.end(); funptr++) {
		Response resp = (this->**funptr)();
		ret.push_back(resp);
	}

	// cleanup
	discard();

	return ret;
}

Response
Client::read_string() {

	Response ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == '$') {
		int sz = ::atol(str.c_str()+1);
		if(sz == -1) {
			return ret; // not found
		}

		if(sz) {
			// need to read last \r\n
			char *buf = new char[sz + 2];

			int remain = sz + 2;
			while(remain) {
				int got = read(m_fd, buf + sz + 2 - remain, remain);
				remain -= got;
			}

			Buffer s;
			s.insert(s.end(), buf, buf+sz);
			delete[] buf;
			// set string.
			ret.type(REDIS_STRING);
			ret.setString(s);
		}
		return ret;
	}

	return ret;
}

std::string
Client::getline() {

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

Response
Client::read_status_code() {
	Response ret(REDIS_BOOL);

	std::string str = getline();
	if(str[0] == '+') {
		ret.setBool(true);
	}
	return ret;
}
Response
Client::read_single_line() {
	Response ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == '+') {
		Buffer s(&str[1]);
		ret.type(REDIS_STRING);
		ret.setString(s);
	}
	return ret;
}

Response
Client::read_queued() {
	Response ret(REDIS_ERR);

	std::string str = getline();
	if(str == "+QUEUED") {
		ret.type(REDIS_QUEUED);
	}
	return ret;
}

Response
Client::read_integer() {
	Response ret(REDIS_ERR);

	std::string str = getline();
	if(str[0] == ':') {
		ret.type(REDIS_LONG);
		ret.setLong(::atol(&str[1]));
	}
	return ret;
}

Response
Client::read_double() {
	Response ret = read_string();
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
Response
Client::read_integer_as_bool() {
	Response ret(REDIS_ERR);

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

Response
Client::read_multi_bulk() {
	Response err(REDIS_ERR);
	Response ret(REDIS_LIST);

	std::string str = getline();
	if(str[0] != '*') {
		return err;
	}
	long count = ::atol(&str[1]);
	if(count <= 0) {
		return ret;
	}
	for(long i = 0; i < count; ++i) {
		Response s = read_string();
		if(s.type() != REDIS_STRING) {
			return err;
		}
		ret.addString(s.string());
	}
	return ret;
}
Response
Client::read_type_reply() {
	Response ret = read_string();
	Response err(REDIS_ERR);

	if(ret.type() != REDIS_STRING) {
		return err;
	}

	ret.type(REDIS_LONG);

	string t = ret.str();
	if(t == "+string") {
		ret.setLong(Client::STRING);
		return ret;
	} else if(t == "+list") {
		ret.setLong(Client::LIST);
		return ret;
	} else if(t == "+set") {
		ret.setLong(Client::SET);
		return ret;
	} else if(t == "+zset") {
		ret.setLong(Client::ZSET);
		return ret;
	} else if(t == "+hash") {
		ret.setLong(Client::HASH);
		return ret;
	}
	return err;
}

/* actual redis commands */

Response
Client::auth(Buffer key) {
	Command cmd("AUTH");

	cmd << key;
	return run(cmd, &Client::read_status_code);
}

Response
Client::select(int index) {
	Command cmd("SELECT");

	cmd << (long)index;
	return run(cmd, &Client::read_status_code);
}

Response
Client::keys(Buffer pattern) {
	Command cmd("KEYS");

	cmd << pattern;
	return run(cmd, &Client::read_multi_bulk);
}
Response
Client::dbsize(int index) {
	Command cmd("DBSIZE");

	cmd << (long)index;
	return run(cmd, &Client::read_integer);
}

Response
Client::lastsave() {
	Command cmd("LASTSAVE");
	return run(cmd, &Client::read_integer);
}
Response
Client::flushdb() {
	Command cmd("FLUSHDB");
	return run(cmd, &Client::read_status_code);
}

Response
Client::flushall() {
	Command cmd("FLUSHALL");
	return run(cmd, &Client::read_status_code);
}
Response
Client::save() {
	Command cmd("SAVE");
	return run(cmd, &Client::read_status_code);
}

Response
Client::bgsave() {
	Command cmd("BGSAVE");
	return run(cmd, &Client::read_status_code);
}

Response
Client::bgrewriteaof() {
	Command cmd("BGREWRITEAOF");
	return run(cmd, &Client::read_status_code);
}

Response
Client::move(Buffer key, int index) {
	Command cmd("MOVE");
	cmd << key << (long)index;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::sort(Buffer key) {
	Command cmd("SORT");
	cmd << key;
	return run(cmd, &Client::read_multi_bulk);
}

Response
Client::sort(Buffer key, SortParams params) {
	Command cmd = params.buildCommand(key);
	return run(cmd, &Client::read_multi_bulk);
}

Response
Client::type(Buffer key) {
	Command cmd("TYPE");
	cmd << key;
	return run(cmd, &Client::read_type_reply);
}

Response
Client::append(Buffer key, Buffer padding) {
	Command cmd("APPEND");
	cmd << key << padding;
	return run(cmd, &Client::read_integer);

}
Response
Client::substr(Buffer key, int start, int end) {
	Command cmd("SUBSTR");
	cmd << key << (long)start << (long)end;
	return run(cmd, &Client::read_string);
}
Response
Client::config(Buffer key, Buffer field) {
	Command cmd("CONFIG");
	cmd << key << Buffer("GET") << field;
	return run(cmd, &Client::read_multi_bulk);
}
Response
Client::config(Buffer key, Buffer field, Buffer val) {
	Command cmd("CONFIG");
	cmd << key << Buffer("SET") << field << val;
	return run(cmd, &Client::read_string);
}

Response
Client::get(Buffer key){
	Command cmd("GET");

	cmd << key;
	return run(cmd, &Client::read_string);
}

Response
Client::set(Buffer key, Buffer val) {
	Command cmd("SET");

	cmd << key << val;
	return run(cmd, &Client::read_status_code);
}

Response
Client::getset(Buffer key, Buffer val) {
	Command cmd("GETSET");

	cmd << key << val;
	return run(cmd, &Client::read_string);
}

Response
Client::setNx(Buffer key, Buffer val) {
	Command cmd("SETNX");
	cmd << key << val;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::exists(Buffer key) {
	Command cmd("EXISTS");
	cmd << key;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::del(Buffer key) {
	Command cmd("DEL");
	cmd << key;
	return run(cmd, &Client::read_integer);
}
Response
Client::del(RedisList keys) {
	return generic_multi_parameter("DEL", keys, &Client::read_integer);
}

Response
Client::mget(RedisList keys) {
	return generic_multi_parameter("MGET", keys, &Client::read_multi_bulk);
}

Response
Client::expire(Buffer key, long ttl) {
	return generic_key_int_return_int("EXPIRE", key, ttl);
}
Response
Client::expireAt(Buffer key, long timestamp) {
	return generic_key_int_return_int("EXPIREAT", key, timestamp);
}

Response
Client::mset(RedisList keys, RedisList vals) {

	return generic_mset("MSET", keys, vals, &Client::read_status_code);
}

Response
Client::msetnx(RedisList keys, RedisList vals) {

	return generic_mset("MSETNX", keys, vals, &Client::read_integer_as_bool);
}

Response
Client::info() {
	Command cmd("INFO");
	return run(cmd, &Client::read_info_reply);
}

Response
Client::read_info_reply() {

	Response ret = read_string();

	if(ret.type() != REDIS_STRING) {
		cout << "FAIL" << endl;
		return Response(REDIS_ERR);
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

Response
Client::incr(Buffer key, int val) {

	return generic_key_int_return_int("INCR", key, val, true);
}
Response
Client::decr(Buffer key, int val) {

	return generic_key_int_return_int("DECR", key, val, true);
}
Response
Client::rename(Buffer src, Buffer dst) {

	Command cmd("RENAME");
	cmd << src << dst;
	return run(cmd, &Client::read_status_code);
}

Response
Client::renameNx(Buffer src, Buffer dst) {

	Command cmd("RENAMENX");
	cmd << src << dst;
	return run(cmd, &Client::read_integer);
}

Response
Client::randomKey() {
	Command cmd("RANDOMKEY");
	return run(cmd, &Client::read_single_line);
}

Response
Client::ttl(Buffer key) {
	Command cmd("TTL");
	cmd << key;
	return run(cmd, &Client::read_integer);
}


Response
Client::ping() {
	Command cmd("PING");
	return run(cmd, &Client::read_status_code);
}

/* List commands */

Response
Client::lpush(Buffer key, Buffer val) {
	return generic_push("LPUSH", key, val);
}
Response
Client::rpush(Buffer key, Buffer val) {
	return generic_push("RPUSH", key, val);
}

Response
Client::rpoplpush(Buffer src, Buffer dst) {

	Command cmd("RPOPLPUSH");
	cmd << src << dst;
	return run(cmd, &Client::read_string);
}

Response
Client::llen(Buffer key) {
	Command cmd("LLEN");
	cmd << key;
	return run(cmd, &Client::read_integer);
}

Response
Client::lpop(Buffer key) {
	return generic_pop("LPOP", key);
}
Response
Client::rpop(Buffer key) {
	return generic_pop("RPOP", key);
}

Response
Client::blpop(RedisList keys, int timeout) {
	return generic_blocking_pop("BLPOP", keys, timeout);
}

Response
Client::brpop(RedisList keys, int timeout) {
	return generic_blocking_pop("BRPOP", keys, timeout);
}


Response
Client::ltrim(Buffer key, int start, int end) {

	Command cmd("LTRIM");
	cmd << key << (long)start << (long)end;
	return run(cmd, &Client::read_status_code);
}

Response
Client::lindex(Buffer key, int pos) {

	Command cmd("LINDEX");
	cmd << key << (long)pos;
	return run(cmd, &Client::read_string);
}

Response
Client::lrem(Buffer key, int count, Buffer val) {

	return generic_list_item_action("LREM", key, count, val, &Client::read_integer);
}

Response
Client::lset(Buffer key, int pos, Buffer val) {

	return generic_list_item_action("LSET", key, pos, val, &Client::read_status_code);
}

Response
Client::lrange(Buffer key, int start, int end) {

	Command cmd("LRANGE");
	cmd << key << (long)start << (long)end;
	return run(cmd, &Client::read_multi_bulk);
}

/* Set commands */

Response
Client::sadd(Buffer key, Buffer val) {
	return generic_set_key_value("SADD", key, val);
}

Response
Client::srem(Buffer key, Buffer val) {
	return generic_set_key_value("SREM", key, val);
}

Response
Client::spop(Buffer key) {
	return generic_pop("SPOP", key);
}

Response
Client::scard(Buffer key) {
	return generic_card("SCARD", key);
}
Response
Client::sismember(Buffer key, Buffer val) {
	return generic_set_key_value("SISMEMBER", key, val);
}

Response
Client::srandmember(Buffer key) {
	return generic_pop("SRANDMEMBER", key);
}

Response
Client::smove(Buffer src, Buffer dst, Buffer member) {

	Command cmd("SMOVE");
	cmd << src << dst << member;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::sinter(RedisList keys) {
	return generic_multi_parameter("SINTER", keys, &Client::read_multi_bulk);
}
Response
Client::sunion(RedisList keys) {
	return generic_multi_parameter("SUNION", keys, &Client::read_multi_bulk);
}
Response
Client::sdiff(RedisList keys) {
	return generic_multi_parameter("SDIFF", keys, &Client::read_multi_bulk);
}
Response
Client::sinterstore(RedisList keys) {
	return generic_multi_parameter("SINTERSTORE", keys, &Client::read_integer);
}
Response
Client::sunionstore(RedisList keys) {
	return generic_multi_parameter("SUNIONSTORE", keys, &Client::read_integer);
}
Response
Client::sdiffstore(RedisList keys) {
	return generic_multi_parameter("SDIFFSTORE", keys, &Client::read_integer);
}

/* zset commands */

Response
Client::zadd(Buffer key, double score, Buffer member) {
	Command cmd("ZADD");

	cmd << key << score << member;
	return run(cmd, &Client::read_integer_as_bool);
}
Response
Client::zrem(Buffer key, Buffer member) {
	Command cmd("ZREM");

	cmd << key << member;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::zincrby(Buffer key, double score, Buffer member) {
	Command cmd("ZINCRBY");

	cmd << key << score << member;
	return run(cmd, &Client::read_double);
}

Response
Client::zscore(Buffer key, Buffer member) {
	Command cmd("ZSCORE");

	cmd << key << member;
	return run(cmd, &Client::read_double);
}
Response
Client::zrank(Buffer key, Buffer member) {
	return generic_zrank("ZRANK", key, member);
}
Response
Client::zrevrank(Buffer key, Buffer member) {
	return generic_zrank("ZREVRANK", key, member);
}
Response
Client::zrange(Buffer key, long start, long end, bool withscores) {
	return generic_zrange("ZRANGE", key, start, end, withscores);
}
Response
Client::zrevrange(Buffer key, long start, long end, bool withscores) {
	return generic_zrange("ZREVRANGE", key, start, end, withscores);
}

Response
Client::zcard(Buffer key) {
	return generic_card("ZCARD", key);
}

Response
Client::zcount(Buffer key, long start, long end) {
	return generic_z_start_end_int("ZCOUNT", key, start, end);
}
Response
Client::zremrangebyrank(Buffer key, long min, long max) {
	return generic_z_start_end_int("ZREMRANGEBYRANK", key, min, max);
}
Response
Client::zremrangebyscore(Buffer key, long min, long max) {
	return generic_z_start_end_int("ZREMRANGEBYSCORE", key, min, max);
}

Response
Client::zrangebyscore(Buffer key, long min, long max, bool withscores) {

	Command cmd("ZRANGEBYSCORE");
	cmd << key << min << max;
	if(withscores) {
		cmd << Buffer("WITHSCORES");
	}

	return run(cmd, &Client::read_multi_bulk);
}

Response
Client::zrangebyscore(Buffer key, long min, long max, long start, long end, bool withscores) {

	Command cmd("ZRANGEBYSCORE");
	cmd << key << min << max << Buffer("LIMIT") << start << end;
	if(withscores) {
		cmd << Buffer("WITHSCORES");
	}

	return run(cmd, &Client::read_multi_bulk);
}

Response
Client::zunion(Buffer key, RedisList keys) {
	vector<double> v;
	return zunion(key, keys, v, "");
}
Response
Client::zunion(Buffer key, RedisList keys, string aggregate) {
	vector<double> v;
	return zunion(key, keys, v, aggregate);
}
Response
Client::zunion(Buffer key, RedisList keys, vector<double> weights) {
	return zunion(key, keys, weights, "");
}
Response
Client::zunion(Buffer key, RedisList keys, vector<double> weights, string aggregate) {
	return generic_z_set_operation("ZUNION", key, keys, weights, aggregate);
}

Response
Client::zinter(Buffer key, RedisList keys) {
	vector<double> v;
	return zunion(key, keys, v, "");
}
Response
Client::zinter(Buffer key, RedisList keys, string aggregate) {
	vector<double> v;
	return zunion(key, keys, v, aggregate);
}
Response
Client::zinter(Buffer key, RedisList keys, vector<double> weights) {
	return zunion(key, keys, weights, "");
}
Response
Client::zinter(Buffer key, RedisList keys, vector<double> weights, string aggregate) {
	return generic_z_set_operation("ZINTER", key, keys, weights, aggregate);
}

Response
Client::generic_z_set_operation(string keyword, Buffer key, RedisList keys,
		vector<double> weights, string aggregate) {

	if(weights.size() != 0 && keys.size() != weights.size()) {
		return Response(REDIS_ERR);
	}
	Command cmd(keyword);
	cmd << key << (long)keys.size();

	RedisList::const_iterator i_key;
	for(i_key = keys.begin(); i_key != keys.end(); i_key++) {
		cmd << *i_key;
	}
	if(weights.size()) {

		cmd << Buffer("WEIGHTS");

		vector<double>::const_iterator w_key;
		for(w_key = weights.begin(); w_key != weights.end(); w_key++) {
			cmd << *w_key;
		}
	}

	if(aggregate.size()) {
		string s = "AGGREGATE " + aggregate;
		cmd << Buffer(s.c_str());
	}

	return run(cmd, &Client::read_integer);
}

/* hash commands */

Response
Client::hset(Buffer key, Buffer field, Buffer val) {
	Command cmd("HSET");
	cmd << key << field << val;
	return run(cmd, &Client::read_integer_as_bool);
}
Response
Client::hget(Buffer key, Buffer field) {
	Command cmd("HSET");
	cmd << key << field;
	return run(cmd, &Client::read_string);
}

Response
Client::hdel(Buffer key, Buffer field) {
	Command cmd("HDEL");
	cmd << key << field;
	return run(cmd, &Client::read_integer_as_bool);
}
Response
Client::hexists(Buffer key, Buffer field) {
	Command cmd("HEXISTS");
	cmd << key << field;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::hlen(Buffer key) {
	Command cmd("HLEN");
	cmd << key;
	return run(cmd, &Client::read_integer);
}
Response
Client::hkeys(Buffer key) {
	return generic_h_simple_list("HKEYS", key);
}

Response
Client::hvals(Buffer key) {
	return generic_h_simple_list("HVALS", key);
}

Response
Client::hgetall(Buffer key) {
	return generic_h_simple_list("HGETALL", key);
}

Response
Client::hincrby(Buffer key, Buffer field, double d) {
	Command cmd("HINCRBY");
	cmd << key << field << d;
	return run(cmd, &Client::read_double);
}

/* generic commands below */

Response
Client::generic_z_start_end_int(string keyword, Buffer key, long start, long end) {

	Command cmd(keyword);
	cmd << key << start << end;
	return run(cmd, &Client::read_integer);
}


Response
Client::generic_zrange(string keyword, Buffer key, long start, long end, bool withscores) {

	Command cmd(keyword);

	cmd << key << start << end;
	if(withscores) {
		cmd << Buffer("WITHSCORES");
	}
	return run(cmd, &Client::read_multi_bulk);
}

Response
Client::generic_zrank(string keyword, Buffer key, Buffer member) {
	Command cmd(keyword);
	cmd << key << member;
	return run(cmd, &Client::read_integer);
}

Response
Client::generic_multi_parameter(string keyword, RedisList &keys, ResponseReader fun) {
	Command cmd(keyword);
	RedisList::const_iterator key;
	for(key = keys.begin(); key != keys.end(); key++) {
		cmd << *key;
	}
	return run(cmd, fun);
}

Response
Client::generic_pop(string keyword, Buffer key){
	Command cmd(keyword);

	cmd << key;
	return run(cmd, &Client::read_string);
}

Response
Client::generic_push(string keyword, Buffer key, Buffer val) {

	Command cmd(keyword);
	cmd << key << val;
	return run(cmd, &Client::read_integer);
}

Response
Client::generic_key_int_return_int(std::string keyword, Buffer key, int val, bool addBy) {

	if(val > 1 && addBy) {
		keyword = keyword + "BY";
	}
	Command cmd(keyword);
	cmd << key;
	if(!addBy || (val > 1 && addBy)) {
		cmd << (long)val;
	}
	return run(cmd, &Client::read_integer);
}

Response
Client::generic_list_item_action(string keyword, Buffer key, int n,
		Buffer val, ResponseReader fun) {
	Command cmd(keyword);
	cmd << key << (long)n << val;

	return run(cmd, fun);
}

Response
Client::generic_set_key_value(string keyword, Buffer key, Buffer val) {

	Command cmd(keyword);
	cmd << key << val;
	return run(cmd, &Client::read_integer_as_bool);
}

Response
Client::generic_card(string keyword, Buffer key) {

	Command cmd(keyword);
	cmd << key;
	return run(cmd, &Client::read_integer);
}

Response
Client::generic_mset(string keyword, RedisList keys, RedisList vals, ResponseReader fun) {

	if(keys.size() != vals.size() || keys.size() == 0) {
		return Response(REDIS_ERR);
	}

	Command cmd(keyword);
	RedisList::const_iterator key;
	RedisList::const_iterator val;
	for(key = keys.begin(), val = vals.begin(); key != keys.end(); key++, val++) {
		cmd << *key << *val;
	}

	return run(cmd, fun);
}

Response
Client::generic_h_simple_list(string keyword, Buffer key) {
	Command cmd(keyword);
	cmd << key;

	return run(cmd, &Client::read_multi_bulk);
}

Response
Client::generic_blocking_pop(string keyword, RedisList keys, int timeout) {

	Command cmd(keyword);
	
	RedisList::const_iterator key;
	for(key = keys.begin(); key != keys.end(); key++) {
		cmd << *key;
	}
	cmd << (long)timeout;

	return run(cmd, &Client::read_multi_bulk);
}
}

