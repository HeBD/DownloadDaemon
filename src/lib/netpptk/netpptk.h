#ifndef NETPPTK_H_
#define NETPPTK_H_

#include "../mt_string/mt_string.h"
#include <boost/thread.hpp>

#ifdef _WIN32
    #include <winsock2.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif


enum ERROR_CODE { SOCKET_CREATION_FAILED, UNKNOWN_HOST, CONNECTION_PROBLEM };

class tkSock {
public:
	/** Basic constructor which creates a socket and eventually calles Windows-init code */
	tkSock();

	/** Fullconstructor
	*	@param MaxConnections The max number of connections
	*	@param MaxReceive The maximal packet size
	*/
	tkSock(const unsigned int MaxConnections, const unsigned int MaxReceive);

	/** Normal destructor to close the socket */
	virtual ~tkSock();

	/** Bind the socket to a port
	*	@param port Port number
	*	@returns True on success
	*/
	bool bind(const int port);

	/** Listen on the specified port
	*	@returns True on success
	*/
	bool listen();

	/** Accept a queried connections
	*	@param new_socket A socket to handle the accepted connection
	*	@returns true on success
	*/
	bool accept(tkSock& new_socket);

	/** Connect to a remote host
	*	@param host Host to connect to
	*	@param port Port to use
	*	@returns True on success
	*/
	bool connect(const std::string host, const int port);

	/** Send data over the socket
	*	@param s Data to send
	*	@returns True on success
	*/
	bool send(const std::string &s);
	bool send(const mt_string &s) { return send(s.c_str()); }
	bool send(const char* s) { return send(std::string(s)); }

	/** Receive data from a socket
	*	@param s String to store the received data
	*	@returns The number of received bytes, negative value on error
	*/
	int recv(std::string &s);
	int recv(mt_string &s) { std::string a; int ret = recv(a); s = a; return ret; }

	/** Convert the object to book to use easy if(..) constructs for validity-checks */
	operator bool() const;

	/** Read data from a string to the socket to send it */
	const tkSock& operator<< (const std::string&);
	const tkSock& operator<< (const mt_string& s) { operator<<(s.c_str()); return *this; }
	const tkSock& operator<< (const char* s) { operator<<(std::string(s)); return *this;}

	/** Read data from a socket and save it in a string */
	const tkSock& operator>> (std::string&);
	const tkSock& operator>> (mt_string &s) { std::string a; recv(a); s = a; return *this; }

	/** Returns the socket descriptor for manual handling */
	int get_sockdesc() const {return m_sock;}

	/** Set a socket descriptor to pass management to the socket
	*	@param n Socket descriptor
	*/
	void set_sockdesc(int n) { m_sock = n;}

	/** Advise the class to handle multithreaded connection management
	*	@param handle A function pointer to a function that wants a tkSock arguement. This function will be called for every connection
	*/
	void auto_accept (void (*handle) (tkSock*));

	/** Stop auto-accepting new connections */
	void auto_accept_stop();

	/** Join the auto-accept threads for terminating programs */
	void auto_accept_join();

	/** Gets the ip-address of the "other side" of the connection
	*	@reutrns Peer ip-address, empty string if it fails
	*/
	std::string get_peer_name();


private:
	void auto_accept_threadfunc(void(*handle) (tkSock*));
	void handle_wrapper(tkSock *sock, void(*handle) (tkSock*));
	int m_sock; // Socket descriptor
	bool valid;
	sockaddr_in m_addr;
	unsigned int m_maxconnections;
	unsigned int m_maxrecv;
	static int m_instanceCount;
	unsigned int m_open_connections;
	boost::thread* auto_accept_thread;
	bool stop_threads;
	std::string append_header(std::string data);
	int remove_header(std::string &data);
};

std::ostream& operator<<(std::ostream& stream, tkSock &sock);
std::istream& operator>>(std::istream& stream, tkSock &sock);




// Exception class

class SocketError {
public:
	SocketError(ERROR_CODE e)
	: error(e) {}

	ERROR_CODE error;

	mt_string what() {
		if(error == SOCKET_CREATION_FAILED)
			return mt_string("Socket creation failed");;
		if(error == UNKNOWN_HOST)
			return mt_string("Unknown Host");
		if(error == CONNECTION_PROBLEM)
			return mt_string("A Problem occured while trying to handle a connection");
		return mt_string("");
	}
};


#endif /*NETPPTK_H_*/
