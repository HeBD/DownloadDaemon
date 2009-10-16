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
#include "../lib/netpptk/netpptk.h"
using namespace std;

void trim_string(std::string &str);
// If password is unknown or should be asked, pass an empty string. The password will be written in it.
void connect_request(tkSock &sock, std::string &host, const std::string &port, std::string &password);
void send_command(tkSock &sock, const std::string &command);

int main(int argc, char* argv[]) {
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
			cout << "\t\t\t\t To find out about possible commands, type \"help\" in the console." << endl;
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
		if(snd.empty()) {
			continue;
		}
		trim_string(snd);
		if(snd == "quit" || snd == "q" || snd == "exit") {
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
			cout << "\texit/quit\t\tdisconnect and exit ddconsole" << endl;
			cout << "\tconnect <host> [port]\tConnect to DownloadDaemon (port is optional)\n" << endl;
			cout << "Commands to be sent to DownloadDaemon:" << endl;
			cout << "General command structure: <target> <command> <parameters> " << endl;
			cout << "Targets are DL for downloads, VAR for configuration variables, FILE for downloaded files, ROUTER for router configuration" << endl;
			cout << "and PREMIUM for premium account setup" << endl;
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
			cout << "PREMIUM LIST\t\t\t returns a list of hosts that support premium setup (only if the plugin supports it)" << endl;
			cout << "PREMIUM SET <host> <user>;<password>\t sets up premium account information for <host>" << endl;
			cout << "PREMIUM GET <host>\t\t returns the username specified for <host>. Passwords can not be retrieved" << endl;
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
		sock << password;
		sock >> snd;
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
	std::string snd = command;
	snd.insert(0, "DDP ");
	sock << snd;
}
