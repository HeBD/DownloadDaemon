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


downloadc::downloadc(){
    mysock = NULL;
}

downloadc::~downloadc(){
    std::lock_guard<std::mutex> lock(mx);
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
        throw client_exception(1, "Connection failed (wrong IP/URL or Port).");
        delete mysock;
        return;
    }


    // authentication
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
            throw client_exception(2, "connection failed (no encryption)");
            delete mysock;
            return;

        }else{ // no encryption permitted but user doesn't care
            // reconnect
            try{
                connection = mysock->connect(host, port);
            }catch(...){} // no code needed here due to boolean connection

            if(!connection){
                throw client_exception(1, "Connection failed (wrong IP/URL or Port).");
                delete mysock;
                return;
            }

            mysock->recv(snd);
            mysock->send(pass);
            mysock->recv(snd);

        }

        // check if password was ok
        if(snd.find("102") == 0 && connection){
            throw client_exception(3, "Wrong Password, Authentication failed.");
            delete mysock;
            return;
        }

    }else{
        throw client_exception(4, "Connection failed");
        delete mysock;
        return;
    }


    // connection ok => save data
    std::lock_guard<std::mutex> lock(mx);

    if(this->mysock != NULL){ //if there is already a connection, delete the old one
        delete this->mysock;
        this->mysock = NULL;
    }

    this->mysock = mysock;
}


// target DL
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
    package mypackage;
    mypackage.id = 0;
    mypackage.name = "";
    bool empty_package = true;

    if(new_content.size() == 0)
        return pkg;

    if((*content_it)[0] != "PACKAGE"){
        throw client_exception(5, "dlist corrupt");
        return pkg;
    }

    for( ; content_it != new_content.end(); content_it++){
        if((*content_it)[0] == "PACKAGE"){ // we have a package line

            if(!empty_package){
                pkg.push_back(mypackage);
                mypackage.id = 0;
                mypackage.name = "";
                mypackage.dls.clear();
            }else
                empty_package = false;

            mypackage.id = atoi((*content_it)[1].c_str());
            try{
                mypackage.name = (*content_it).at(2);
            }catch(...){
                mypackage.name = "";
            }

        }else{ // we have a download line
            download dl;

            // defaults
            dl.id = 0;
            dl.date = "";
            dl.title = "";
            dl.url = "";
            dl.status = "";
            dl.downloaded = 0;
            dl.size = 0;
            dl.wait = 0;
            dl.error = "";
            dl.speed = 0;

            try{
                dl.id = atoi((*content_it).at(0).c_str());
                dl.date = (*content_it).at(1);
                dl.title = (*content_it).at(2);
                dl.url = (*content_it).at(3);
                dl.status = (*content_it).at(4);
                dl.downloaded = atol((*content_it).at(5).c_str());
                dl.size = atol((*content_it).at(6).c_str());
                dl.wait = atoi((*content_it).at(7).c_str());
                dl.error = (*content_it).at(8);
                dl.speed = atoi((*content_it).at(9).c_str());
            }catch(...){}

            mypackage.dls.push_back(dl);
        }
    }

    pkg.push_back(mypackage);

    return pkg;
}


void downloadc::add_download(int package, std::string url, std::string title){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

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
            throw client_exception(6, "failed to create package");
            return;
        }
        package = id;
    }

    pkg_exists << package;

    // finally adding the download
    mysock->send("DDP DL ADD " + pkg_exists.str() + " " + url + " " + title);
    mysock->recv(answer);

    check_error_code(answer);
}


void downloadc::delete_download(int id, file_delete fdelete){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;
    bool error = false;

    // test if there is a file on the server
    mysock->send("DDP FILE GETPATH " + id_str.str());
    mysock->recv(answer);

    if(!answer.empty() && (fdelete == dont_know)){ // file exists and user didn't decide to delete
        throw client_exception(7, "file exists, call again with del_file or dont_delete");
        return;
    }

    if(!answer.empty() && (fdelete == del_file)){ // file exists and should be deleted
        mysock->send("DDP DL DEACTIVATE " + id_str.str());
        mysock->recv(answer);

        mysock->send("DDP FILE DEL " + id_str.str());
        mysock->recv(answer);

        if(answer.find("109") == 0){ // 109 FILE <-- file operation on a file that does not exist
            error = true; // exception will be thrown later so we can still delete the download
        }
    }

    mysock->send("DDP DL DEL " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);

    if(error){
        throw client_exception(8, "deleting file failed");
        return;
    }
}


void downloadc::stop_download(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL STOP " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);
}


void downloadc::priority_up(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL UP " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);
}


void downloadc::priority_down(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL DOWN " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);
}


void downloadc::activate_download(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL ACTIVATE " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);
}


void downloadc::deactivate_download(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL DEACTIVATE " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);
}


// target PKG
std::vector<package> downloadc::get_packages(){
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
    package mypackage;
    mypackage.id = 0;
    mypackage.name = "";
    bool empty_package = true;

    if(new_content.size() == 0)
        return pkg;

    if((*content_it)[0] != "PACKAGE"){
        throw client_exception(5, "dlist corrupt");
        return pkg;
    }

    for( ; content_it != new_content.end(); content_it++){
        if((*content_it)[0] == "PACKAGE"){ // we have a package line

            if(!empty_package){
                pkg.push_back(mypackage);
                mypackage.id = 0;
                mypackage.name = "";
                mypackage.dls.clear();
            }else
                empty_package = false;

            mypackage.id = atoi((*content_it)[1].c_str());
            try{
                mypackage.name = (*content_it).at(2);
            }catch(...){
                mypackage.name = "";
            }

        }else{ // we have a download line which we don't want
        }
    }

    pkg.push_back(mypackage);

    return pkg;
}


int downloadc::add_package(std::string name){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    mysock->send("DDP PKG ADD " + name);
    mysock->recv(answer);

    int id = atoi(answer.c_str());
    if(id == -1){
        throw client_exception(9, "failed to create package");
    }
    return id;
}


void downloadc::delete_package(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG DEL " + id_str.str());
    mysock->recv(answer);
}


void downloadc::package_priority_up(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG UP " + id_str.str());
    mysock->recv(answer);
}


void downloadc::package_priority_down(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG DOWN " + id_str.str());
    mysock->recv(answer);
}


bool downloadc::package_exists(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG EXISTS " + id_str.str());
    mysock->recv(answer);

    if(answer == "0")
        return false;

    return true;
}


// target VAR
void downloadc::set_var(std::string var, std::string value, std::string old_value ){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    if(var == "mgmt_password")
        value = old_value + " ; " + value;

    mysock->send("DDP VAR SET " + var + " = " + value);
    mysock->recv(answer);

    check_error_code(answer);
}


std::string downloadc::get_var(std::string var){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    mysock->send("DDP VAR GET " + var);
    mysock->recv(answer);

    return answer;
}


// target FILE
void downloadc::delete_file(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP FILE GETPATH " + id_str.str());
    mysock->recv(answer);

    if(answer.empty()) // no error if file doesn't exist
        return;

    mysock->send("DDP DL DEACTIVATE " + id_str.str());
    mysock->recv(answer);

    mysock->send("DDP FILE DEL " + id_str.str());
    mysock->recv(answer);

    check_error_code(answer);
}


std::string downloadc::get_file_path(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP FILE GETPATH " + id_str.str());
    mysock->recv(answer);

    return answer;
}


uint64_t downloadc::get_file_size(int id){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP FILE GETSIZE " + id_str.str());
    mysock->recv(answer);

    return atol(answer.c_str());
}


// target ROUTER
std::vector<std::string> downloadc::get_router_list(){
    check_connection();

    std::string model_list, line = "";
    size_t lineend = 1;

    mx.lock();
    mysock->send("DDP ROUTER LIST");
    mysock->recv(model_list);
    mx.unlock();

    std::vector<std::string> router_list;

    // parse lines
    while(model_list.length() > 0 && lineend != std::string::npos){
        lineend = model_list.find("\n"); // termination character for line

        if(lineend == std::string::npos){ // this is the last line (which ends without \n)
            line = model_list.substr(0, model_list.length());
            model_list = "";

        }else{ // there is still another line after this one
            line = model_list.substr(0, lineend);
            model_list = model_list.substr(lineend+1);
        }

        router_list.push_back(line);
    }

    return router_list;
}


void downloadc::set_router_model(std::string model){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP ROUTER SETMODEL " + model);
    mysock->recv(answer);

    check_error_code(answer);
}


void downloadc::set_router_var(std::string var, std::string value){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP ROUTER SET " + var + " = " + value);
    mysock->recv(answer);

    check_error_code(answer);
}


std::string downloadc::get_router_var(std::string var){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP ROUTER GET " + var);
    mysock->recv(answer);

    return answer;
}


// target PREMIUM
std::vector<std::string> downloadc::get_premium_list(){
    check_connection();

    std::string host_list, line = "";
    size_t lineend = 1;

    mx.lock();
    mysock->send("DDP PREMIUM LIST");
    mysock->recv(host_list);
    mx.unlock();

    std::vector<std::string> premium_list;

    // parse lines
    while(host_list.length() > 0 && lineend != std::string::npos){
        lineend = host_list.find("\n"); // termination character for line

        if(lineend == std::string::npos){ // this is the last line (which ends without \n)
            line = host_list.substr(0, host_list.length());
            host_list = "";

        }else{ // there is still another line after this one
            line = host_list.substr(0, lineend);
            host_list = host_list.substr(lineend+1);
        }

        premium_list.push_back(line);
    }

    return premium_list;
}


void downloadc::set_premium_var(std::string host, std::string user, std::string password){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP PREMIUM SET " + host + " " + user + ";" + password);
    mysock->recv(answer);

    check_error_code(answer);
}


std::string downloadc::get_premium_var(std::string host){
    check_connection();

    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP PREMIUM GET " + host);
    mysock->recv(answer);

    return answer;
}


// helper functions
void downloadc::check_connection(){
    std::lock_guard<std::mutex> lock(mx);

    if(mysock == NULL){ // deleted mysock
        throw client_exception(10, "connection lost");
    }

    std::string answer;

    mysock->send("hello :3"); // send something, so we can test the connection
    mysock->recv(answer);

    if(!*mysock){ // if there is no active connection
        throw client_exception(10, "connection lost");
    }
}


void downloadc::check_error_code(std::string check_me){
    if(check_me.find("101") == 0){ // 101 PROTOCOL <-- instruction invalid
        throw client_exception(11, "syntax error");
        return;
    }

    if(check_me.find("102") == 0){ // 102 AUTHENTICATION <-- Authentication failed (wrong password or wrong encryption)
        throw client_exception(12, "Authentication failed");
        return;
    }

    if(check_me.find("103") == 0){ // 103 URL <-- Invalid URL
        throw client_exception(13, "invalid URL");
        return;
    }

    if(check_me.find("104") == 0){ // 104 ID <-- Entered a not-existing ID or moved top-download up or bottom download down
        throw client_exception(14, "nonexisting ID");
        return;
    }

    if(check_me.find("105") == 0){ // 105 STOP <-- If a download could not be stopped, because it's not running
        throw client_exception(15, "not running");
        return;
    }

    if(check_me.find("106") == 0){ // 106 ACTIVATE <-- If you try to activate a download that is already active
        throw client_exception(16, "already activated");
        return;
    }

    if(check_me.find("107") == 0){ // 107 DEACTIVATE <-- If you try to deactivate a downoad that is already unactive
        throw client_exception(17, "already deactivated");
        return;
    }

    if(check_me.find("108") == 0){ // 108 VARIABLE     <-- If the variable you tried to set is invalid
        throw client_exception(18, "variable invalid");
        return;
    }

    if(check_me.find("109") == 0){ // 109 FILE <-- If you do any file operation on a file that does not exist.
        throw client_exception(19, "file does not exist");
        return;
    }

    if(check_me.find("110") == 0){ // 110 PERMISSION <-- If a file could not be written / no write permission to list-file
        throw client_exception(20, "no write permission");
        return;
    }

    if(check_me.find("111") == 0){ // 111 VALUE    <-- If you change a config-variable to an invalid value
        throw client_exception(21, "invalid value");
        return;
    }

}
