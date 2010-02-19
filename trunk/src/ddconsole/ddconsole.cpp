/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <netpptk/netpptk.h>
#include <crypt/md5.h>
using namespace std;

void trim_string(std::string &str);
// If password is unknown or should be asked, pass an empty string. The password will be written in it.
void connect_request(tkSock &sock, std::string &host, const std::string &port, std::string &password);
void send_command(tkSock &sock, const std::string &command);

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");
	vector<string> args;
	for(int i = 1; i < argc; ++i) {
		args.push_back(argv[i]);
	}

	string host;
	string port("56789");
	string password;
	string command;
	for(vector<string>::iterator it = args.begin(); it != args.end(); ++it) {
		if(*it == "--help") {
			cout << "Usage: " << argv[0] << " [options]" << endl;
			cout << "Possible options:" << endl;
			cout << "\t--host <host>\t\t Specifies the host to connect to [localhost]" << endl;
			cout << "\t--port <port>\t\t Specifies the port to use [56789]" << endl;
			cout << "\t--password <password>\t Specifies the password to connect" << endl;
			cout << "\t--command <command(s)>\t Specifies a command to send after connectiong. The client will close after execution." << endl;
			cout << "\t\t\t\t To find out about possible commands, type \"help\" in the console or visit" << endl;
			cout << "\t\t\t\t http://downloaddaemon.sourceforge.net for more information." << endl;
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
	if(!host.empty()) {
		connect_request(sock, host, port, password);
		if(!sock) {
			cout << "Unable to connect." << endl;
		}
	}

	trim_string(command);
	if(!command.empty()) {
		std::string answer;
		while(command.find('\n') != string::npos) {
			std::string to_send = command.substr(0, command.find('\n'));
			trim_string(to_send);
			send_command(sock, to_send);
			sock >> answer;
			cout << answer << endl;
			command = command.substr(command.find('\n') + 1);
		}
		trim_string(command);
		send_command(sock, command);
		sock >> answer;
		cout << answer << endl;
		return 0;
	}

	std::string snd;
	while(true) {
		cout << host << "> ";
		if(command.empty()) {
			getline(cin, snd);
		} else {
			snd = command;
		}
		if(cin.fail() || cin.bad()) {
			exit(0);
		}
		if(snd.empty()) {
			continue;
		}
		trim_string(snd);
		if(snd == "quit" || snd == "q" || snd == "exit" || snd == "\f" || snd == "\0") {
			return 0;
		} else if(snd.find("connect") == 0) {
			snd = snd.substr(7);
			trim_string(snd);
			host = snd.substr(0, snd.find_first_of("\n\t "));
			snd = snd.substr(host.length());
			trim_string(snd);

			port = "56789";
			if(!snd.empty()) {
				port = snd;
			}
			connect_request(sock, host, port, password);
			if(!sock) {
				cout << "Unable to connect." << endl;
				host = "";
			}
			continue;
		}

		if(snd == "help" || snd == "?") {
			cout << "There are commands for ddconsole and there are commands for a connected DownloadDaemon." << endl;
			cout << "Commands for ddconsole:" << endl;
			cout << setw(30) << left << "exit/quit" << "disconnect and exit ddconsole" << endl;
			cout << setw(30) << left << "connect <host> [port]" << "Connect to DownloadDaemon (port is optional)\n" << endl;
			cout << "Commands to be sent to DownloadDaemon (command is not case sensitive, parameters are case sensitive):" << endl;
			cout << "General command structure: <target> <command> <parameters> " << endl;
			cout << "Targets are DL for downloads, PKG for packages, VAR for configuration variables, FILE for downloaded files, " << endl;
			cout << "ROUTER for router configuration and PREMIUM for premium account setup" << endl;
			cout << "Possible commands:" << endl;
			cout << setw(30) << left << "DL LIST" << "Dumps the current download list" << endl;
			cout << setw(30) << left << "DL ADD <package id> <url> <comment>" << "Add a download" << endl;
			cout << setw(30) << left << "DL DEL <package id>.<id>" << "Deletes a download (get ID from DL LIST)" << endl;
			cout << setw(30) << left << "DL STOP <package id>.<id>" << "Stops a download" << endl;
			cout << setw(30) << left << "DL UP <package id>.<id>" << "moves a download up" << endl;
			cout << setw(30) << left << "DL DOWN <package id>.<id>" << "moves a download down" << endl;
			cout << setw(30) << left << "DL ACTIVATE <package id>.<id>" << "activates a download" << endl;
			cout << setw(30) << left << "DL DEACTIVATE <package id>.<id>" << "deactivates a download" << endl;
			cout << setw(30) << left << "PKG ADD <name>" << "adds a new package to the list"<< endl;
			cout << setw(30) << left << "PKG DEL <id>" << "removes a package and all downloads inside it"<< endl;
			cout << setw(30) << left << "PKG UP <id>" << "moves a package up"<< endl;
			cout << setw(30) << left << "PKG DOWN <id>" << "moves a package down"<< endl;
			cout << setw(30) << left << "VAR SET <var>=<value>" << "sets a configuration variable (<variable> needs to have the identifier of a variable in downloaddaemon.conf)" << endl;
			cout << setw(30) << left << "VAR GET <var>" << "get the value of a configuration variable" << endl;
			cout << setw(30) << left << "FILE DEL <id>" << "delete the file corresponding to the given download id" << endl;
			cout << setw(30) << left << "FILE GETPATH <id>" << "returns the path on the local filesystem of the downloaded file or an empty string if it does not exsist yet" << endl;
			cout << setw(30) << left << "FILE GETSIZE <id>" << "returns the file-size of the file corresponding to the given ID" << endl;
			cout << setw(30) << left << "ROUTER LIST" << "returns a list of supported router models" << endl;
			cout << setw(30) << left << "ROUTER SETMODEL <model>" << "sets the router model" << endl;
			cout << setw(30) << left << "ROUTER SET <var>=<value>" << "sets a router configuration varialbe" << endl;
			cout << setw(30) << left << "ROUTER GET <var>" << "returns the value of a router configuration variable" << endl;
			cout << setw(30) << left << "PREMIUM LIST" << "returns a list of hosts that support premium setup (only if the plugin supports it)" << endl;
			cout << setw(30) << left << "PREMIUM SET <host> <usr>;<pw>" << "sets up premium account information for <host>" << endl;
			cout << setw(30) << left << "PREMIUM GET <host>" << "returns the username specified for <host>. Passwords can not be retrieved" << endl;
			cout << "\n\nVisit http://downloaddaemon.sourceforge.net for more information." << endl;
			continue;
		}

		string answer;
		send_command(sock, snd);
		sock >> answer;

		if(!sock) {
			host = "";
			cout << "Disconnected from DownloadDaemon" << endl;
		}

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

void connect_request(tkSock &sock, std::string &host, const std::string &port, std::string &password) {
	if(host.empty() || !sock.connect(host, atoi(port.c_str()))) {
		host = "";
		return;
	}

	std::string snd;
	sock >> snd;
	if(snd.find("100") == 0) {
		cout << "No Password." << endl;
	} else if (snd.find("102") == 0) {
		if(password.empty()) {
			cout << "Password: ";
			getline(cin, password);
		}
		trim_string(password);

		// try md5 authentication, then ask for plain authentication.
		sock << "ENCRYPT";
		std::string rnd;
		sock.recv(rnd);
		rnd += password;
		if(rnd.find("102") != 0) {
			MD5_CTX md5;
			MD5_Init(&md5);
			unsigned char* enc_data = new unsigned char[rnd.length()];
			for(size_t i = 0; i < rnd.length(); ++i) {
				enc_data[i] = rnd[i];
			}

			MD5_Update(&md5, enc_data, rnd.length());
			unsigned char result[16];
			MD5_Final(result, &md5);
			std::string enc_passwd((char*)result, 16);
			delete [] enc_data;
			sock.send(enc_passwd);
			sock.recv(snd);
		} else {
			cout << "Encrypted authentication not supported by server. Do you want to try unsecure plain-text authentication? (y/n) ";
			std::string do_plain;
			cin >> do_plain;
			if(do_plain == "y" || do_plain == "Y") {
				if(!sock.connect(host, atoi(port.c_str()))) {
					host = "";
					return;
				}
				sock.recv(snd);
				sock.send(password);
				sock.recv(snd);
			}
		}


		if(snd.find("100") == 0) {
			cout << "Authentication success" << endl;
		} else if(snd.find("102") == 0) {
			cout << "Unable to authenticate" << endl;
			exit(-2);
		} else {
			cout << "Unknown error while authentication" << endl;
			exit(-3);
		}
	} else {
		cout << "Unknown error while authentication. Either the connection was closed while authenticating or you connected"
				"to an unsupported service." << endl;
		exit(-3);
	}
}

void send_command(tkSock &sock, const std::string &command) {
	std::string snd(command);

	// make first 2 words upper case
	size_t pos = snd.find_first_of(" \n\r\t\0", snd.find_first_of(" \n\r\t\0") + 1);
	string::iterator end_it = snd.begin() + pos;
	std::transform(snd.begin(), pos == string::npos ? snd.end() : end_it, snd.begin(), ::toupper);
	snd.insert(0, "DDP ");
	sock << snd;
}
