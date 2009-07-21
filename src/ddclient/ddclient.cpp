#include <iostream>
#include "../lib/netpptk.h"
using namespace std;

int main() {
	tkSock sock;
	sock.connect("127.0.0.1", 4321);
	std::string snd;
	sock >> snd;
	cout << "Auth: " << snd << endl;
	while(sock) {
		cout << "Senden: ";
		getline(cin, snd);
		sock << snd;
		sock >> snd;
		cout << "Antwort: " << snd << endl;

	}

}
