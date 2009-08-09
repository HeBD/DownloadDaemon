#include <iostream>
#include <vector>
#include <string>
#include "../lib/netpptk/netpptk.h"
using namespace std;

void trim_string(std::string &str);

int main(int argc, char* argv[]) {
	vector<string> args;
	for(int i = 1; i < argc; ++i) {
		args.push_back(argv[i]);
	}

	string host("127.0.0.1");
	string port("56789");
	string password;
	string command;
	for(vector<string>::iterator it = args.begin(); it != args.end(); ++it) {
		if(*it == "--help") {
			cout << "Usage: " << argv[0] << " [options]" << endl;
			cout << "Possible options:" << endl;
			cout << "\t--host <host>\t Specifies the host to connect to [localhost]" << endl;
			cout << "\t--port <port>\t Specifies the port to use [56789]" << endl;
			cout << "\t--password <password>\t Specifies the password to connect" << endl;
			cout << "\t--command <commant>\t Specifies a command to send after connectiong. The client will close then" << endl;
			return 0;
		}
		if(*it == "--host") {
			++it;
			host = *it;
		} else if (*it == "--port") {
			++it;
			port = *it;
		} else if (*it == "--password") {
			++it;
			password = *it;
		} else if(*it == "--command") {
			++it;
			command = *it;
		}
	}

	tkSock sock;
	if(!sock.connect(host, atoi(port.c_str()))) {
		cout << "Unable to connect to DownloadDaemon";
		return -1;
	}

	std::string snd;

	sock >> snd;
	if(snd.find("100") == 0) {
		cout << "No Password" << endl;
	} else if (snd.find("102") == 0) {
		if(password.empty()) {
			cout << "Password: ";
			getline(cin, password);
		}
		sock << password;
		sock >> snd;
		if(snd.find("100") == 0) {
			cout << "Authentication success" << endl;
		} else if(snd.find("102") == 0) {
			cout << "Unable to authenticate" << endl;
			return -1;
		} else {
			cout << "Unknown error while authentication" << endl;
			return -1;
		}
	} else {
		cout << "Unknown error while authentication" << endl;
		return -1;
	}

	while(sock) {
		cout << "> ";
		if(command.empty()) {
			getline(cin, snd);
		} else {
			snd = command;
		}
		if(snd.empty()) {
			continue;
		}
		trim_string(snd);
		if(snd == "quit" || snd == "q" || snd == "exit") {
			return 0;
		}

		if(snd == "help" || snd == "?") {
			cout << "General command structure: <target> <command> <parameters> " << endl;
			cout << "Targets are DL for downloads, VAR for configuration variables, FILE for downloaded files and ROUTER for router configuration" << endl;
			cout << "Possible commands:" << endl;
			cout << "DL LIST\t\t\t\t Dumps the current download list" << endl;
			cout << "DL ADD <url> <comment>\t\t Add a download" << endl;
			cout << "DL DEL <id>\t\t\t Deletes a download (get ID from DL LIST)" << endl;
			cout << "DL STOP <id>\t\t\t Stops a download" << endl;
			cout << "DL UP <id>\t\t\t moves a download up" << endl;
			cout << "DL DOWN <id>\t\t\t moves a download down" << endl;
			cout << "DL ACTIVATE <id>\t\t activates a download" << endl;
			cout << "DL DEACTIVATE <id>\t\t deactivates a download" << endl;
			cout << "VAR SET <var>=<value>\t sets a configuration variable (<variable> needs to have the identifier of a variable in downloaddaemon.conf)" << endl;
			cout << "VAR GET <var>\t\t get the value of a configuration variable" << endl;
			cout << "FILE DEL <id>\t\t\t delete the file corresponding to the given download id" << endl;
			cout << "FILE GETPATH <id>\t\t returns the path on the local filesystem of the downloaded file or an empty string if it does not exsist yet" << endl;
			cout << "FILE GETSIZE <id>\t\t returns the file-size of the file corresponding to the given ID" << endl;
			cout << "ROUTER LIST\t\t returns a list of supported router models" << endl;
			cout << "ROUTER SETMODEL <model>\t sets the router model" << endl;
			cout << "ROUTER SET <var>=<value>\t sets a router configuration varialbe" << endl;
			cout << "ROUTER GET <var>\t\t returns the value of a router configuration variable" << endl;
			cout << "\nTo quit ddclient, type exit or quit" << endl;
			continue;
		}
		string answer;
		snd.insert(0, "DDP ");
		sock << snd;
		sock >> answer;

		cout << answer << endl;
		if(!command.empty()) {
		    break;
		}
	}
	cout << endl;

}

void trim_string(std::string &str) {
	while(str.length() > 0 && isspace(str[0])) {
		str.erase(str.begin());
	}
	while(str.length() > 0 && isspace(*(str.end() - 1))) {
		str.erase(str.end() -1);
	}
}
