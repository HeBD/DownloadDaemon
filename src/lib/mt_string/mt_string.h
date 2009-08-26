#ifndef mt_string_H_INCLUDED
#define mt_string_H_INCLUDED

#include<string>
#include<iostream>
#include<boost/thread.hpp>

class mt_string {
public:

	class iterator {
	// NOT THREADSAFE YET! todo if needed!
	public:
		iterator();
		iterator(std::string::iterator it);
		iterator operator++();
		iterator operator--();
		iterator operator++(int i);
		iterator operator--(int i);
		char operator*();
		char* operator->();
		iterator operator+(size_t i);
		iterator operator-(size_t i);
		iterator operator+=(size_t i);
		iterator operator -=(size_t i);
		bool operator==(iterator it);
		bool operator!=(iterator it);
		operator std::string::iterator();

	private:
		std::string::iterator string_iterator;
	};

	explicit mt_string ( );
	mt_string ( const mt_string& str );
	mt_string ( const mt_string& str, size_t pos, size_t n = npos );
	mt_string ( const char * s, size_t n );
	mt_string ( const char * s );
	mt_string ( size_t n, char c );
	mt_string ( const std::string &str);
	~mt_string ();

	mt_string& operator= ( const mt_string& str );
	mt_string& operator= ( const char* s );
	mt_string& operator= ( char c );

	bool operator==(mt_string &str);
	bool operator==(const char *str);

	operator std::string();


	iterator begin();
	iterator end();
	size_t size();
	size_t length();
	void clear();
	bool empty ( );
	char& operator[]( size_t pos );
	char& at ( size_t pos );
	mt_string& operator+= ( const mt_string& str );
	mt_string& operator+= ( const char* s );
	mt_string& operator+= ( char c );
	mt_string& append ( mt_string& str );
	mt_string& append ( mt_string& str, size_t pos, size_t n );
	mt_string& append ( const char* s, size_t n );
	mt_string& append ( const char* s );
	mt_string& append ( size_t n, char c );
	void push_back ( char c );
	mt_string& insert ( size_t pos1, mt_string& str );
	mt_string& insert ( size_t pos1, mt_string& str, size_t pos2, size_t n );
	mt_string& insert ( size_t pos1, const char* s, size_t n);
	mt_string& insert ( size_t pos1, const char* s );
	mt_string& insert ( size_t pos1, size_t n, char c );
	mt_string& erase ( size_t pos = 0, size_t n = npos );
	iterator erase ( iterator position );
	iterator erase ( iterator first, iterator last );
	mt_string& replace ( size_t pos1, size_t n1,   const mt_string& str );
	mt_string& replace ( iterator i1, iterator i2, const mt_string& str );
	mt_string& replace ( size_t pos1, size_t n1, const mt_string& str, size_t pos2, size_t n2 );
	mt_string& replace ( size_t pos1, size_t n1,   const char* s, size_t n2 );
	mt_string& replace ( iterator i1, iterator i2, const char* s, size_t n2 );
	mt_string& replace ( size_t pos1, size_t n1,   const char* s );
	mt_string& replace ( iterator i1, iterator i2, const char* s );
	mt_string& replace ( size_t pos1, size_t n1,   size_t n2, char c );
	mt_string& replace ( iterator i1, iterator i2, size_t n2, char c );

	const char* c_str ( ) const;
	size_t find ( mt_string& str, size_t pos = 0 ) const;
	size_t find ( const char* s, size_t pos, size_t n ) const;
	size_t find ( const char* s, size_t pos = 0 ) const;
	size_t find ( char c, size_t pos = 0 ) const;
	size_t find_first_of ( mt_string& str, size_t pos = 0 ) const;
	size_t find_first_of ( const char* s, size_t pos, size_t n ) const;
	size_t find_first_of ( const char* s, size_t pos = 0 ) const;
	size_t find_first_of ( char c, size_t pos = 0 ) const;
	size_t find_last_of ( mt_string& str, size_t pos = npos ) const;
	size_t find_last_of ( const char* s, size_t pos, size_t n ) const;
	size_t find_last_of ( const char* s, size_t pos = npos ) const;
	size_t find_last_of ( char c, size_t pos = npos ) const;
	mt_string substr ( size_t pos = 0, size_t n = npos ) const;


	static size_t npos;
	std::string real_string;
};


bool operator== ( const mt_string& lhs, const mt_string& rhs );
bool operator== ( const char* lhs, const mt_string& rhs );
bool operator== ( const mt_string& lhs, const char* rhs );

bool operator!= ( const mt_string& lhs, const mt_string& rhs );
bool operator!= ( const char* lhs, const mt_string& rhs );
bool operator!= ( const mt_string& lhs, const char* rhs );

bool operator< ( const mt_string& lhs, const mt_string& rhs );
bool operator< ( const char* lhs, const mt_string& rhs );
bool operator< ( const mt_string& lhs, const char* rhs );

bool operator> ( const mt_string& lhs, const mt_string& rhs );
bool operator> ( const char* lhs, const mt_string& rhs );
bool operator> ( const mt_string& lhs, const char* rhs );

bool operator<= ( const mt_string& lhs, const mt_string& rhs );
bool operator<= ( const char* lhs, const mt_string& rhs );
bool operator<= ( const mt_string& lhs, const char* rhs );

bool operator>= ( const mt_string& lhs, const mt_string& rhs );
bool operator>= ( const char* lhs, const mt_string& rhs );
bool operator>= ( const mt_string& lhs, const char* rhs );

mt_string operator+ (const mt_string& lhs, const mt_string& rhs);
mt_string operator+ (const char* lhs, const mt_string& rhs);
mt_string operator+ (char lhs, const mt_string& rhs);
mt_string operator+ (const mt_string& lhs, const char* rhs);
mt_string operator+ (const mt_string& lhs, char rhs);

std::ostream& operator<< (std::ostream& os, const mt_string& str);
std::istream& operator>> (std::istream& is, mt_string& str);
std::istream& getline ( std::istream& is, mt_string& str, char delim );
std::istream& getline ( std::istream& is, mt_string& str );


#endif // mt_string_H_INCLUDED
