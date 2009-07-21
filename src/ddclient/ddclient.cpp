#include <iostream>
#include "../lib/netpptk.h"
using namespace std;

int main(int argc, char* argv[]) {
	tkSock sock;
	sock.connect("127.0.0.1", 4321);
	std::string snd;
	sock >> snd;
	cout << "Auth: " << snd << endl;
	while(sock) {
		cout << "Senden: ";
		if(argc == 2) {
			sock << argv[1];
			sock >> snd;
			cout << "Antwort: " << snd << endl;
			return 0;
		}
		getline(cin, snd);
		sock << snd;
		sock >> snd;
		cout << "Antwort: " << snd << endl;

	}

}
