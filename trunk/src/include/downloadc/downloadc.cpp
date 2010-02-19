/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "downloadc.h"
#include <crypt/md5.h>
#include <sstream>

/// mind TODOs! <=========================================================================================== !

downloadc::downloadc(){
	mysock = NULL;
}

downloadc::~downloadc(){
	boost::mutex::scoped_lock lock(mx);
	delete mysock;
	mysock = NULL;
}


void downloadc::connect(std::string host, int port, std::string pass, bool encrypt){
	tkSock *mysock = new tkSock();
	bool connection = false;

	try{
	   connection = mysock->connect(host, port);
	}catch(...){} // no code needed here due to boolean connection

	if(!connection){
		// TODO: thow exception: connection failed (wrong ip/url or port)
		delete mysock;
		return;
	}


	// authentification
	std::string snd;
	mysock->recv(snd);

	if(snd.find("100") == 0){ // 100 SUCCESS <-- Operation succeeded
		// nothing to do here if you reach this

	}else if(snd.find("102") == 0){ // 102 AUTHENTICATION <-- Authentication failed
		// try md5 authentication
		mysock->send("ENCRYPT");
		std::string rnd;
		mysock->recv(rnd); // random bytes

		if(rnd.find("102") != 0) { // encryption permitted
			rnd += pass;

			MD5_CTX md5;
			MD5_Init(&md5);

			unsigned char *enc_data = new unsigned char[rnd.length()];
			for(size_t i = 0; i < rnd.length(); ++i){ // copy random bytes from string to cstring
				enc_data[i] = rnd[i];
			}

			MD5_Update(&md5, enc_data, rnd.length());
			unsigned char result[16];
			MD5_Final(result, &md5); // put md5hash in result
			std::string enc_passwd((char*)result, 16);
			delete [] enc_data;

			mysock->send(enc_passwd);
			mysock->recv(snd);

		}else if(encrypt){ // encryption not permitted and user really wants it
			// TODO: thow exception: connection failed (no encryption)
			delete mysock;
			return;

		}else{ // no encryption permitted but user doesn't care
			// reconnect
			try{
				connection = mysock->connect(host, port);
			}catch(...){} // no code needed here due to boolean connection

			if(!connection){
				// TODO: thow exception: connection failed (wrong ip/url or port)
				delete mysock;
				return;
			}

			mysock->recv(snd);
			mysock->send(pass);
			mysock->recv(snd);

		}

		// check if password was ok
		if(snd.find("102") == 0 && connection){
			// TODO: thow exception: connection failed (wrong password)
			delete mysock;
			return;
		}

	}else{
		// TODO: thow exception: connection failed (unknown error)
		delete mysock;
		return;
	}


	// connection ok => save data
	boost::mutex::scoped_lock lock(mx);

	if(this->mysock != NULL){ //if there is already a connection, delete the old one
		delete this->mysock;
		this->mysock = NULL;
	}

	this->mysock = mysock;
}


std::vector<package> downloadc::get_list(){
	check_connection();

	std::string answer;

	mx.lock();
	mysock->send("DDP DL LIST");
	mysock->recv(answer);
	mx.unlock();

	std::vector<std::string> splitted_line;
	std::vector<std::vector<std::string> > new_content;
	std::string line, tab;
	size_t lineend = 1, tabend = 1;

	// parse lines
	while(answer.length() > 0 && lineend != std::string::npos){
		lineend = answer.find("\n"); // termination character for line
		line = answer.substr(0, lineend);
		answer = answer.substr(lineend+1);

		// parse columns
		tabend = 0;

		while(line.length() > 0 && tabend != std::string::npos){

			tabend = line.find("|"); // termination character for column

			if(tabend == std::string::npos){ // no | found, so it is the last column
				tab = line;
				line = "";
			}else{
				if(tabend != 0 && line.at(tabend-1) == '\\') // because titles can have | inside (will be escaped with \)
					tabend = line.find("|", tabend+1);

				tab = line.substr(0, tabend);
				line = line.substr(tabend+1);

			}
		splitted_line.push_back(tab); // save all tabs per line for later use
		}

		new_content.push_back(splitted_line);
		splitted_line.clear();
	}


	// now we have the data in a vector<vector<string> > and can push it in a better readable structure
	std::vector<package> pkg;
	std::vector<std::vector<std::string> >::iterator content_it = new_content.begin();
	package mypackage = {0, ""};
	bool empty_package = true;

	if((*content_it)[0] != "PACKAGE"){
		// TODO: thow exception: dlist corrupt
		return pkg;
	}

	for( ; content_it != new_content.end(); content_it++){
		if((*content_it)[0] == "PACKAGE"){ // we have a package line

			if(!empty_package){
				pkg.push_back(mypackage);
				mypackage.id = 0;
				mypackage.name = "";
			}else
				empty_package = false;

			mypackage.id = atoi((*content_it)[1].c_str());
			mypackage.name = (*content_it)[2];


		}else{ // we have a download line
			download dl;
			dl.id = atoi((*content_it)[0].c_str());
			dl.date = (*content_it)[1];
			dl.title = (*content_it)[2];
			dl.url = (*content_it)[3];
			dl.status = (*content_it)[4];
			dl.downloaded = atol((*content_it)[5].c_str());
			dl.size = atol((*content_it)[6].c_str());
			dl.wait = atoi((*content_it)[7].c_str());
			dl.error = (*content_it)[8];
			dl.speed = atoi((*content_it)[9].c_str());

			mypackage.dls.push_back(dl);
		}
	}

	return pkg;
}


void downloadc::add_download(int package, std::string url, std::string title){
	check_connection();

	boost::mutex::scoped_lock lock(mx);

	// make sure everything with the package is ok
	std::string answer;
	std::stringstream pkg, pkg_exists;
	pkg << package;

	mysock->send("DDP PKG EXISTS " + pkg.str());
	mysock->recv(answer);

	int id = atoi(answer.c_str());
	if(id == 0){ // package doesn't exist => create empty package
		mysock->send("DDP PKG ADD");
		mysock->recv(answer);

		id = atoi(answer.c_str());
		if(id == -1){
			// TODO: thow exception: failed to create package
			return;
		}
		package = id;
	}

	pkg_exists << package;

	// finally adding the download
	mysock->send("DDP DL ADD " + pkg_exists.str() + " " + url + " " + title);
	mysock->recv(answer);

	if(answer.find("103") == 0){ // 103 URL <-- Invalid URL
		// TODO: thow exception: invalid url
	}
}


void downloadc::add_package(std::string name){
	check_connection();

	std::string answer;

	mx.lock();
	mysock->send("DDP PKG ADD " + name);
	mysock->recv(answer);
	mx.unlock();

	int id = atoi(answer.c_str());
	if(id == -1){
		// TODO: thow exception: failed to create package
	}
}


void downloadc::check_connection(){
	boost::mutex::scoped_lock lock(mx);

	if(mysock == NULL || !*mysock || mysock->get_peer_name() == ""){ // if there is no active connection
		// TODO: thow exception: connection lost
	}
}
