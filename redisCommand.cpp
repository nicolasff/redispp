#include "redisCommand.h"

#include <iostream>
#include <sstream>
#include <string.h>

using namespace std;

namespace redis {

Command::Command(string keyword) {
	
	Buffer s;
	s.insert(s.end(), keyword.begin(), keyword.end());
	m_elements.push_back(s);
}

Command &
Command::operator<<(const Buffer s) {
	m_elements.push_back(s);

	return *this;
}

// TODO: refactor the operator<< for numeric types.
Command&
Command::operator<<(long l) {

	// copy long value into a string stream
	stringstream ss;
	ss << l;
	string str = ss.str();

	// copy string stream into a char* buffer
	Buffer s;
	s.insert(s.end(), str.begin(), str.end());

	// add to the list of elements
	m_elements.push_back(s);

	return *this;
}

Command&
Command::operator<<(double d) {

	// copy long value into a string stream
	stringstream ss;
	ss << d;
	string str = ss.str();

	// copy string stream into a char* buffer
	Buffer s;
	s.insert(s.end(), str.begin(), str.end());

	// add to the list of elements
	m_elements.push_back(s);

	return *this;
}

Buffer
Command::get() {

	Buffer ret;

	stringstream ss_count;
	ss_count << "*" << m_elements.size() << "\r\n";
	string str_count = ss_count.str();

	// add header line to return buffer.
	ret.insert(ret.end(), str_count.begin(), str_count.end());


	// add each element, preceded by its size.
	list<Buffer>::const_iterator i;
	for(i = m_elements.begin(); i != m_elements.end(); i++) {
		// add "size" size followed by \r\n
		stringstream ss_size;
		ss_size << "$" << i->size() << "\r\n";
		string ss_str = ss_size.str();
		ret.insert(ret.end(), ss_str.begin(), ss_str.end());
		
		// add element, followed by CRLF.
		ret.insert(ret.end(), i->begin(), i->end());
		ret.push_back('\r');
		ret.push_back('\n');
	}

	return ret;
}
}

