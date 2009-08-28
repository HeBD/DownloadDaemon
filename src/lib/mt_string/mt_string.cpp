#include "mt_string.h"
#include <string>

// Anonymous namespace to make it file-global
namespace {
	boost::mutex mt_string_mutex;
}

size_t mt_string::npos = std::string::npos;

// Iterator
mt_string::iterator::iterator() {}
mt_string::iterator::iterator(std::string::iterator it) { string_iterator = it; }
mt_string::iterator mt_string::iterator::operator++() { ++string_iterator; return *this; }
mt_string::iterator mt_string::iterator::operator--() { --string_iterator; return *this; }
mt_string::iterator mt_string::iterator::operator++(int i) { mt_string::iterator tmp = *this; ++string_iterator; return tmp; }
mt_string::iterator mt_string::iterator::operator--(int i) { mt_string::iterator tmp = *this; --string_iterator; return tmp; }
char mt_string::iterator::operator*() { return *string_iterator; }
char* mt_string::iterator::operator->() { return &(*string_iterator); }
mt_string::iterator mt_string::iterator::operator+(size_t i) { mt_string::iterator it(*this); it.string_iterator += i; return it; }
mt_string::iterator mt_string::iterator::operator-(size_t i) { mt_string::iterator it(*this); it.string_iterator -= i; return it; }
mt_string::iterator mt_string::iterator::operator+=(size_t i) { string_iterator += i; return *this; }
mt_string::iterator mt_string::iterator::operator -=(size_t i) { string_iterator -= i; return *this; }
bool mt_string::iterator::operator==(iterator it) { return string_iterator == it.string_iterator; }
bool mt_string::iterator::operator!=(iterator it) { return  string_iterator != it.string_iterator; }
mt_string::iterator::operator std::string::iterator() { return string_iterator; }


// mt_string
mt_string::mt_string ( ) {}
mt_string::mt_string ( const mt_string& str ) : real_string(str.real_string) {}
mt_string::mt_string ( const mt_string& str, size_t pos, size_t n) : real_string(str.real_string, pos, n) {}
mt_string::mt_string ( const char * s, size_t n ) : real_string(s, n) {}
mt_string::mt_string ( const char * s ) : real_string(s) {}
mt_string::mt_string ( size_t n, char c ) : real_string(n, c) {}
mt_string::mt_string ( const std::string &str) : real_string(str) {}
mt_string::~mt_string () {  }

mt_string& mt_string::operator= ( const mt_string& str ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string = str.real_string; return *this;}
mt_string& mt_string::operator= ( const char* s ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string = s; return *this; }
mt_string& mt_string::operator= ( char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string = c; return *this; }

bool mt_string::operator==(mt_string &str) { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string == str.real_string; }
bool mt_string::operator==(const char *str) { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string == str; }

mt_string::operator std::string() { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string; }


mt_string::iterator mt_string::begin() { boost::mutex::scoped_lock lock(mt_string_mutex); return iterator(real_string.begin()); }
mt_string::iterator mt_string::end() { boost::mutex::scoped_lock lock(mt_string_mutex); return iterator(real_string.end()); }
size_t mt_string::size() { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.size(); }
size_t mt_string::length() { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.length(); }
void mt_string::clear() { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.clear(); }
bool mt_string::empty ( ) { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.empty(); }
char& mt_string::operator[] ( size_t pos ) { boost::mutex::scoped_lock lock(mt_string_mutex);  return real_string[pos];	}
char& mt_string::at ( size_t pos ) { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.at(pos); }
mt_string& mt_string::operator+= ( const mt_string& str ) { boost::mutex::scoped_lock lock(mt_string_mutex);real_string += str.real_string; return *this; }
mt_string& mt_string::operator+= ( const char* s ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string += s; return *this; }
mt_string& mt_string::operator+= ( char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string += c; return *this; }
mt_string& mt_string::append ( mt_string& str ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.append(str.real_string); return *this; }
mt_string& mt_string::append ( mt_string& str, size_t pos, size_t n ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.append(str.real_string, pos, n); return *this; }
mt_string& mt_string::append ( const char* s, size_t n ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.append(s, n); return *this; }
mt_string& mt_string::append ( const char* s ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.append(s); return *this; }
mt_string& mt_string::append ( size_t n, char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.append(n, c); return *this; }
void mt_string::push_back ( char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.push_back(c); }
mt_string& mt_string::insert ( size_t pos1, mt_string& str ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.insert(pos1, str.real_string); return *this; }
mt_string& mt_string::insert ( size_t pos1, mt_string& str, size_t pos2, size_t n ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.insert(pos1, str.real_string, pos2, n); return *this; }
mt_string& mt_string::insert ( size_t pos1, const char* s, size_t n) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.insert(pos1, s, n); return *this; }
mt_string& mt_string::insert ( size_t pos1, const char* s ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.insert(pos1, s); return *this; }
mt_string& mt_string::insert ( size_t pos1, size_t n, char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.insert(pos1, n, c); return *this; }
mt_string& mt_string::erase ( size_t pos, size_t n ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.erase(pos, n); return *this; }
mt_string::iterator mt_string::erase ( iterator position ) { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.erase(position); }
mt_string::iterator mt_string::erase ( iterator first, iterator last ) { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.erase(first, last); }
mt_string& mt_string::replace ( size_t pos1, size_t n1,   const mt_string& str ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(pos1, n1, str.real_string); return *this; }
mt_string& mt_string::replace ( iterator i1, iterator i2, const mt_string& str ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(i1, i2, str.real_string); return *this; }
mt_string& mt_string::replace ( size_t pos1, size_t n1, const mt_string& str, size_t pos2, size_t n2 ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(pos1, n1, str.real_string, pos2, n2); return *this; }
mt_string& mt_string::replace ( size_t pos1, size_t n1,   const char* s, size_t n2 ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(pos1, n1, s, n2); return *this; }
mt_string& mt_string::replace ( iterator i1, iterator i2, const char* s, size_t n2 ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(i1, i2, s, n2); return *this; }
mt_string& mt_string::replace ( size_t pos1, size_t n1,   const char* s ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(pos1, n1, s); return *this; }
mt_string& mt_string::replace ( iterator i1, iterator i2, const char* s ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(i1, i2, s); return *this; }
mt_string& mt_string::replace ( size_t pos1, size_t n1,   size_t n2, char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(pos1, n1, n2, c); return *this; }
mt_string& mt_string::replace ( iterator i1, iterator i2, size_t n2, char c ) { boost::mutex::scoped_lock lock(mt_string_mutex); real_string.replace(i1, i2, n2, c); return *this; }

const char* mt_string::c_str ( ) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.c_str(); }
size_t mt_string::find ( mt_string& str, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find(str.real_string, pos); }
size_t mt_string::find ( const char* s, size_t pos, size_t n ) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find(s, pos, n); }
size_t mt_string::find ( const char* s, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find(s, pos); }
size_t mt_string::find ( char c, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find(c, pos); }
size_t mt_string::find_first_of ( mt_string& str, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_first_of(str.real_string, pos); }
size_t mt_string::find_first_of ( const char* s, size_t pos, size_t n ) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_first_of(s, pos, n); }
size_t mt_string::find_first_of ( const char* s, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_first_of(s, pos); }
size_t mt_string::find_first_of ( char c, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_first_of(c, pos); }
size_t mt_string::find_last_of ( mt_string& str, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_last_of(str.real_string, pos); }
size_t mt_string::find_last_of ( const char* s, size_t pos, size_t n ) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_last_of(s, pos, n); }
size_t mt_string::find_last_of ( const char* s, size_t pos) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_last_of(s, pos); }
size_t mt_string::find_last_of ( char c, size_t pos ) const { boost::mutex::scoped_lock lock(mt_string_mutex); return real_string.find_last_of(c, pos); }
mt_string mt_string::substr ( size_t pos, size_t n ) const { boost::mutex::scoped_lock lock(mt_string_mutex); return mt_string(real_string.substr(pos, n));}

bool operator== ( const mt_string& lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string == rhs.real_string; }
bool operator== ( const char* lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs == rhs.real_string; }
bool operator== ( const mt_string& lhs, const char* rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string == rhs; }

bool operator!= ( const mt_string& lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string != rhs.real_string; }
bool operator!= ( const char* lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs != rhs.real_string; }
bool operator!= ( const mt_string& lhs, const char* rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string != rhs; }

bool operator< ( const mt_string& lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string < rhs.real_string; }
bool operator< ( const char* lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs < rhs.real_string; }
bool operator< ( const mt_string& lhs, const char* rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string < rhs; }

bool operator> ( const mt_string& lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string > rhs.real_string; }
bool operator> ( const char* lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs > rhs.real_string; }
bool operator> ( const mt_string& lhs, const char* rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string > rhs; }

bool operator<= ( const mt_string& lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string <= rhs.real_string; }
bool operator<= ( const char* lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs <= rhs.real_string; }
bool operator<= ( const mt_string& lhs, const char* rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string <= rhs; }

bool operator>= (const mt_string& lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string >= rhs.real_string; }
bool operator>= ( const char* lhs, const mt_string& rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs >= rhs.real_string; }
bool operator>= ( const mt_string& lhs, const char* rhs ) { boost::mutex::scoped_lock lock(mt_string_mutex); return lhs.real_string >= rhs; }

mt_string operator+ ( const mt_string& lhs,  const mt_string& rhs) { boost::mutex::scoped_lock lock(mt_string_mutex); return mt_string(lhs.real_string + rhs.real_string); }
mt_string operator+ (const char* lhs, const mt_string& rhs) { boost::mutex::scoped_lock lock(mt_string_mutex); return mt_string(lhs + rhs.real_string); }
mt_string operator+ (char lhs, const mt_string& rhs) { boost::mutex::scoped_lock lock(mt_string_mutex); return mt_string(lhs + rhs.real_string); }
mt_string operator+ ( const mt_string& lhs, const char* rhs) { boost::mutex::scoped_lock lock(mt_string_mutex); return mt_string(lhs.real_string + rhs); }
mt_string operator+ ( const mt_string& lhs, char rhs) { boost::mutex::scoped_lock lock(mt_string_mutex); return mt_string(lhs.real_string + rhs); }

std::ostream& operator<< (std::ostream& os, const mt_string& str) { boost::mutex::scoped_lock lock(mt_string_mutex); os << str.real_string; return os;}
std::istream& operator>> (std::istream& is, mt_string& str) { boost::mutex::scoped_lock lock(mt_string_mutex); is >>str.real_string; return is;}
std::istream& getline ( std::istream& is, mt_string& str, char delim ) { boost::mutex::scoped_lock lock(mt_string_mutex); std::getline(is, str.real_string, delim); return is; }
std::istream& getline ( std::istream& is, mt_string& str )  { boost::mutex::scoped_lock lock(mt_string_mutex); std::getline(is, str.real_string); return is; }
