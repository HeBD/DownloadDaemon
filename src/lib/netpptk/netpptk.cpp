/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "netpptk.h"
#include <sstream>
#include <cstring>
#include <vector>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#ifdef _WIN32
	#include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <io.h>
    #define sleep(x) Sleep(x * 1000)
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

// Static socket-instance counter (needed for the Windows-init, and cleanup code)
int tkSock::m_instanceCount = 0;


// Windows XP and before does not definet inet_ntop...
#ifdef _WIN32
const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt) {
        if (af == AF_INET) {
                struct sockaddr_in in;
                memset(&in, 0, sizeof(in));
                in.sin_family = AF_INET;
                memcpy(&in.sin_addr, src, sizeof(struct in_addr));
                getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
                return dst;
        }
        else if (af == AF_INET6) {
                struct sockaddr_in6 in;
                memset(&in, 0, sizeof(in));
                in.sin6_family = AF_INET6;
                memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
                getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
                return dst;
        }
        return NULL;
}
#endif


tkSock::tkSock() : m_maxconnections(20), m_maxrecv(1024), m_open_connections(0), auto_accept_thread(0) {
	valid = false;
	#ifdef _WIN32
		if(m_instanceCount == 0) {
			WORD Version;
			WSADATA wsaData;
			Version = MAKEWORD(1, 1);
			if(WSAStartup(Version, &wsaData) != 0) {
				throw SocketError(SOCKET_CREATION_FAILED);
			}
		}
	#endif
	m_sock = ::socket(AF_INET, SOCK_STREAM, 0);

	if(m_sock <= 0) {
		throw SocketError(SOCKET_CREATION_FAILED);
	}

	#ifndef _WIN32
		int y = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y));
	#endif
	++m_instanceCount;
}

tkSock::tkSock(const unsigned int MaxConnections, const unsigned int MaxReceive)
	: m_maxconnections(MaxConnections), m_maxrecv(MaxReceive), m_open_connections(0), auto_accept_thread(0) {
	valid = false;
	#ifdef _WIN32
		WORD Version;
		WSADATA wsaData;
		Version = MAKEWORD(1, 1);
		if(WSAStartup(Version, &wsaData) != 0) {

			throw SocketError(SOCKET_CREATION_FAILED);
		}
	#endif

	if(m_maxrecv > 9999) {
		m_maxrecv = 9999;
	}
	m_sock = ::socket(AF_INET, SOCK_STREAM, 0);

	if(m_sock < 0) {
		throw SocketError(SOCKET_CREATION_FAILED);
	}
	#ifndef _WIN32
		int y = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
	#endif
	++m_instanceCount;
}

tkSock::~tkSock() {
	#ifdef _WIN32
		closesocket(m_sock);
		if(m_instanceCount == 0) {
			WSACleanup();
		}
	#else
		close(m_sock);
	#endif
	if(auto_accept_thread != 0) {
		delete auto_accept_thread;
	}
	--m_instanceCount;
}

bool tkSock::bind(const int port) {
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	addrinfo *res;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	std::stringstream ss;
	ss << port;
	if(getaddrinfo(NULL, ss.str().c_str(), &hints, &res) != 0) {
		valid = false;
		return false;
	}

	int bind_return = ::bind(m_sock, res->ai_addr, sizeof(*res));
	if(bind_return < 0) {
		freeaddrinfo(res);
		valid = false;
		return false;
	}
	freeaddrinfo(res);
	valid = true;
	return true;
}

bool tkSock::listen() {
	int listen_return = ::listen(m_sock, m_maxconnections);
	if(listen_return == -1) {
	    valid = false;
		return false;
	}
	valid = true;
	return true;
}

bool tkSock::accept (tkSock& new_socket) {
    #ifdef _WIN32
        int addr_length = sizeof(m_addr);
    #else
        socklen_t addr_length = sizeof(m_addr);
    #endif
    close(new_socket.m_sock);
	new_socket.m_sock = ::accept(this->m_sock, reinterpret_cast<sockaddr*>(&m_addr), &addr_length);
	if(new_socket.m_sock <= 0) {
		new_socket.valid = false;
		return false;
	}
	new_socket.valid = true;
	return true;
}

void tkSock::auto_accept (void (*handle) (tkSock*)) {
	auto_accept_thread = new boost::thread(boost::bind(&tkSock::auto_accept_threadfunc, this, handle));
}

void tkSock::auto_accept_threadfunc(void (*handle) (tkSock*)) {
	stop_threads = false;
	while(!stop_threads) {
		if(m_open_connections < m_maxconnections) {
			++m_open_connections;
			tkSock* tmpsock;
			try {
				tmpsock = new tkSock(1, m_maxrecv);
			} catch (...) {
				--m_open_connections;
				continue;
			}

			if(!this->accept(*tmpsock)) {
				--m_open_connections;
				delete tmpsock;
				continue;
			}
			if(stop_threads) {
				delete tmpsock;
				break;
			}
			boost::thread connection_handle(boost::bind(&tkSock::handle_wrapper, this, tmpsock, handle));
		} else {
			sleep(1);
		}
	}
}

void tkSock::handle_wrapper(tkSock *tmpsock, void(*handle) (tkSock*)) {
	(*handle) (tmpsock);
	delete tmpsock;
	--m_open_connections;
}

void tkSock::auto_accept_join() {
	auto_accept_thread->join();
}

void tkSock::auto_accept_stop() {
	delete auto_accept_thread;
	stop_threads = true;
	auto_accept_thread = 0;
}

bool tkSock::connect(const std::string host, const int port) {
	struct hostent *host_info;
	unsigned long addr;
	if((addr = inet_addr(host.c_str())) != INADDR_NONE) {
		memcpy(reinterpret_cast<char*>(&m_addr.sin_addr), &addr, sizeof(addr));
	}
	else {
		host_info = gethostbyname(host.c_str());
		if(host_info == NULL) {
			valid = false;
			throw SocketError(UNKNOWN_HOST);
		}

		memcpy(reinterpret_cast<char*>(&m_addr.sin_addr), host_info->h_addr, host_info->h_length);
	}
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);

	int status = ::connect(m_sock, reinterpret_cast<sockaddr*>(&m_addr), sizeof(m_addr));
	if(status == 0) {
		valid = true;
		return true;
	}
	valid = false;
	return false;
}

bool tkSock::send(const std::string &s) {
	int status = 0;
	std::string s_new;
	s_new = append_header(s);
	do {
		if(!valid) {
			return false;
		}
		status += ::send(m_sock, s_new.c_str() + status, s_new.size() - status, MSG_NOSIGNAL);
		if(status < 0) {
			valid = false;
			return false;
		}
	} while(s_new.size() - status > 0);
	//valid = true;
	return true;
}

int tkSock::recv(std::string& s) {
	char initbuf[21];
	s = "";
	memset(initbuf, 0, 21);
	int status = ::recv(m_sock, initbuf, 21, MSG_NOSIGNAL);
 	if(status <= 0) {
		valid = false;
		return 0;
	}
	s.append(initbuf, status);
	int msgLen = remove_header(s);
	if(msgLen == -1) {
		s = "";
		valid = false;
		return status;
	} else if(s.length() >= (unsigned)msgLen) {
		return status;
	}

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof(tv))) {
		valid = false;
		return status;
	}

	char *buf = new char[m_maxrecv + 1];
	while(s.length() < (unsigned)msgLen) {
		memset(buf, 0, m_maxrecv + 1);
		int old_status = status;
		status += ::recv(m_sock, buf, m_maxrecv, 0);
		if(status <= old_status) {
			valid = false;
			tv.tv_sec = 0;
			setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
			return 0;
		}
		s.append(buf, status - old_status);
	}
	tv.tv_sec = 0;
	setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	delete [] buf;
	if(s.size() > static_cast<unsigned>(msgLen)) {
		return 0;
	}

	return status;
}

std::string tkSock::get_peer_name() {
	if(!valid) {
		return "";
	}
	struct sockaddr_storage name;

	#ifdef _WIN32
		int size = sizeof(name);
	#else
		socklen_t size = sizeof(name);
	#endif

	if(getpeername(m_sock, (sockaddr*)&name, &size) != 0) {
		valid = false;
		return "";
	}
	char ipstr[INET6_ADDRSTRLEN];


	if (name.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&name;
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&name;
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}
	return ipstr;
}

std::string tkSock::append_header(std::string data) {
	int len = data.size();
	std::stringstream ss;
	ss << len;
	std::string lenstr;
	ss >> lenstr;
	data = lenstr + ':' + data;
	return data;
}

int tkSock::remove_header(std::string &data) {
	size_t seperator_pos = data.find(':');
	if(seperator_pos == std::string::npos) {
		return -1;
	}
	std::string NumberString;
	int retval;
	NumberString = data.substr(0, seperator_pos);
	std::stringstream ss;
	ss << NumberString;
	ss >> retval;
	data.erase(0, NumberString.length() + 1);
	return retval;
}

tkSock::operator bool() const {
	return ((m_sock > 0) && (valid == true));
}

const tkSock& tkSock::operator<< (const std::string& s) {
	tkSock::send(s);
	return *this;
}

const tkSock& tkSock::operator>> (std::string& s) {
	tkSock::recv(s);
	return *this;
}

std::ostream& operator<<(std::ostream& stream, tkSock &sock) {
	std::string data;
	sock.recv(data);

	stream << data;
	return stream;
}

std::istream& operator>>(std::istream& stream, tkSock &sock) {
	std::string data;
	stream >> data;
	sock.send(data);
	return stream;
}
