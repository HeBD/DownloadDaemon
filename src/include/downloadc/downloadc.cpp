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


downloadc::downloadc() : skip_update(false), term(false){
    mysock = NULL;
}


downloadc::~downloadc(){
    std::lock_guard<std::mutex> lock(mx);
    delete mysock;
    mysock = NULL;
}


void downloadc::set_term(){
    term = true;
}


void downloadc::connect(std::string host, int port, std::string pass, bool encrypt){
    tkSock *mysock = new tkSock();
    bool connection = false;

    try{
       connection = mysock->connect(host, port);
    }catch(...){} // no code needed here due to boolean connection

    if(!connection){
        delete mysock;
        throw client_exception(1, "Connection failed (wrong IP/URL or Port).");
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
            delete mysock;
            throw client_exception(2, "connection failed (no encryption)");
            return;

        }else{ // no encryption permitted but user doesn't care
            // reconnect
            try{
                connection = mysock->connect(host, port);
            }catch(...){} // no code needed here due to boolean connection

            if(!connection){
                delete mysock;
                throw client_exception(1, "Connection failed (wrong IP/URL or Port).");
                return;
            }

            mysock->recv(snd);
            mysock->send(pass);
            mysock->recv(snd);

        }

        // check if password was ok
        if(snd.find("102") == 0 && connection){
            delete mysock;
            throw client_exception(3, "Wrong Password, Authentication failed.");
            return;
        }

    }else{
        delete mysock;
        throw client_exception(4, "Connection failed");
        return;
    }


    // connection ok => save data
    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    if(this->mysock != NULL){ //if there is already a connection, delete the old one
        delete this->mysock;
        this->mysock = NULL;
    }

    this->mysock = mysock;
    skip_update = false;
}


std::vector<update_content> downloadc::get_updates(){
    check_connection();

    std::string answer;
    std::vector<std::string> all_answers, splitted_line;
    std::vector<std::string>::reverse_iterator rit;
    std::vector<int> ids;
    std::vector<int>::iterator id_it;
    std::vector<update_content> updates;
    int id;
    bool push;

    while(true){
        if(term) // terminate signal
            return updates;

        if(!skip_update){
            mx.lock();

            for(size_t i = 0; i < old_updates.size(); i++){ // copy all old updates into the working vector all_answers
                all_answers.push_back(old_updates[i]);
            }
            old_updates.clear();


            while(mysock->select(0)){
                mysock->recv(answer);
                all_answers.push_back(answer);
            }

            mx.unlock();

            if(all_answers.size() > 0)
                break;
        }

        sleep(1);
    }

    // delete messages from the queue
    for(rit = all_answers.rbegin(); rit != all_answers.rend(); ++rit){
        push = true;

        // if there is more then one download update from the same id we only need the last one
        if(rit->find("SUBS_DOWNLOADS:UPDATE") == 0){

            splitted_line = this->split_string(*rit, ":");
            if(splitted_line.size() < 4) // corrupt line
                continue;

            id = atoi(splitted_line[3].c_str());

            for(id_it = ids.begin(); id_it != ids.end(); ++id_it){
                if(id == *id_it){
                    *rit = "";
                    push = false;
                    break;

                }
            }

            if(push)
                ids.push_back(id);
        }
    }

    // delete empty lines
    for(unsigned int i = 0; i < all_answers.size(); i++){
        if(all_answers[i] == ""){
            all_answers.erase(all_answers.begin()+i);
            i--;
        }

    }

    // now we have to pack the vector into a nicer structure
    std::vector<std::string>::iterator it = all_answers.begin();
    update_content update;
    size_t pos;

    /* Lines look like this:
     * SUBS_CONFIG:var=value
     * SUBS_DOWNLOADS:UPDATE:package_id:id|date|title|...
     * SUBS_DOWNLOADS:NEW:PACKAGE|id|name|...
     */

    for(; it != all_answers.end(); ++it){

        if(it->find("SUBS_CONFIG") == 0){
            update.sub = SUBS_CONFIG;
            answer = it->substr(12); // cut away SUBS_CONFIG:

            if((pos = answer.find("=")) == std::string::npos) // corrupt line
                continue;

            update.var_name = answer.substr(0, pos);
            update.value = answer.substr(pos+1);
            
            trim_string(update.var_name);
            trim_string(update.value);
            
            updates.push_back(update);
            continue;
        }

        splitted_line = this->split_string(*it, "|", true);


        if(splitted_line[0].find("SUBS_DOWNLOADS") == 0){
            update.sub = SUBS_DOWNLOADS;
            answer = splitted_line[0].substr(15); // cut away SUBS_DOWNLOAD:

            if(answer.find("UPDATE") == 0){
                update.reason = UPDATE;
                answer = answer.substr(7);
            }else if(answer.find("NEW") == 0){
                update.reason = NEW;
                answer = answer.substr(4);
            }else if(answer.find("DELETE") == 0){
                update.reason = DELETE;
                answer = answer.substr(7);
            }else if(answer.find("MOVEUP") == 0){
                update.reason = MOVEUP;
                answer = answer.substr(7);
            }else if(answer.find("MOVEDOWN") == 0){
                update.reason = MOVEDOWN;
                answer = answer.substr(9);
            }else // corrupt line or don't know reason_type
                continue;

            if(answer.size() <= 0)
                continue; // corrupt line

            if(answer == "PACKAGE"){
                update.package = true;

                if(splitted_line.size() < 2) // corrupt line
                    continue;

                update.id = atoi(splitted_line[1].c_str());
                try{
                    update.name = splitted_line.at(2);
                }catch(...){
                    update.name = "";
                }

                try{
                    update.password = splitted_line.at(3);
                }catch(...){
                    update.password = "";
                }

            }else{ // we're looking at a download line
                update.package = false;

                // defaults
                update.pkg_id = -1;
                update.id = -1;
                update.date = "";
                update.title = "";
                update.url = "";
                update.status = "";
                update.downloaded = 0;
                update.size = 0;
                update.wait = 0;
                update.error = "";
                update.speed = 0;

                if((pos = answer.find(":")) != std::string::npos){
                    update.pkg_id = atoi(answer.c_str()); // answer should look like this: 1:25, so pkg_id is 1
                    answer = answer.substr(pos+1);

                }else
                    continue; // corrupt line

                try{
                    update.id = atoi(answer.c_str());
                    update.date = splitted_line.at(1);
                    update.title = splitted_line.at(2);
                    update.url = splitted_line.at(3);
                    update.status = splitted_line.at(4);
                    update.downloaded = std::strtod(splitted_line.at(5).c_str(), NULL);
                    update.size = std::strtod(splitted_line.at(6).c_str(), NULL);
                    update.wait = atoi(splitted_line.at(7).c_str());
                    update.error = splitted_line.at(8);
                    update.speed = atoi(splitted_line.at(9).c_str());
                }catch(...){}


            }
        }else // corrupt line or don't know subs_type
            continue;

        if(update.id != -1)
            updates.push_back(update);
    }

    return updates;
}


// target DL
std::vector<package> downloadc::get_list(){
    check_connection();

    std::string answer;

    skip_update = true;
    mx.lock();
    mysock->send("DDP DL LIST");
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    mx.unlock();
    skip_update = false;

    std::vector<std::vector<std::string> > new_content;
    this->split_special_string(new_content, answer);

    // now we have the data in a vector<vector<string> > and can push it in a better readable structure
    std::vector<package> pkg;
    std::vector<std::vector<std::string> >::iterator content_it = new_content.begin();
    package mypackage;
    mypackage.id = 0;
    mypackage.name = "";
    mypackage.password = "";
    bool empty_package = true;

    if(new_content.size() == 0)
        return pkg;

    if((*content_it)[0] != "PACKAGE"){
        throw client_exception(5, "dlist corrupt");
        return pkg;
    }

    for( ; content_it != new_content.end(); content_it++){
        if(content_it->size() < 1) // ignore blank lines
            continue;

        if((*content_it)[0] == "PACKAGE"){ // we have a package line

            if(!empty_package){
                pkg.push_back(mypackage);
                mypackage.id = 0;
                mypackage.name = "";
                mypackage.password = "";
                mypackage.dls.clear();
            }else
                empty_package = false;

            mypackage.id = atoi((*content_it)[1].c_str());
            try{
                mypackage.name = (*content_it).at(2);
            }catch(...){
                mypackage.name = "";
            }

            try{
                mypackage.password = (*content_it).at(3);
            }catch(...){
                mypackage.password = "";
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
                dl.downloaded = std::strtod((*content_it).at(5).c_str(), NULL);
                dl.size = std::strtod((*content_it).at(6).c_str(), NULL);
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

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    // make sure everything with the package is ok
    std::string answer;
    std::stringstream pkg, pkg_exists;
    pkg << package;

    mysock->send("DDP PKG EXISTS " + pkg.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    int id = atoi(answer.c_str());
    if(id == 0){ // package doesn't exist => create empty package
        mysock->send("DDP PKG ADD");
        mysock->recv(answer);
        while(!check_correct_answer(answer))
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
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::delete_download(int id, file_delete fdelete){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;
    bool error = false;

    // test if there is a file on the server
    mysock->send("DDP FILE GETPATH " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    if(!answer.empty() && (fdelete == dont_know)){ // file exists and user didn't decide to delete
        throw client_exception(7, "file exists, call again with del_file or dont_delete");
        return;
    }

    if(!answer.empty() && (fdelete == del_file)){ // file exists and should be deleted
        mysock->send("DDP DL DEACTIVATE " + id_str.str());
        mysock->recv(answer);
        while(!check_correct_answer(answer))
            mysock->recv(answer);

        mysock->send("DDP FILE DEL " + id_str.str());
        mysock->recv(answer);
        while(!check_correct_answer(answer))
            mysock->recv(answer);

        if(answer.find("109") == 0){ // 109 FILE <-- file operation on a file that does not exist
            error = true; // exception will be thrown later so we can still delete the download
        }
    }

    mysock->send("DDP DL DEL " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);

    if(error){
        throw client_exception(8, "deleting file failed");
        return;
    }
}


void downloadc::stop_download(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL STOP " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::priority_up(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL UP " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::priority_down(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL DOWN " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::activate_download(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL ACTIVATE " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::deactivate_download(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP DL DEACTIVATE " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::set_download_var(int id, std::string var, std::string value){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;
    std::stringstream s;
    s << id;

    mysock->send("DDP DL SET " + s.str() + " " + var + "=" + value);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer); // set DL_URL can throw 108 variable if download is running or finished
}


std::string downloadc::get_download_var(int id, std::string var){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;
    std::stringstream s;
    s << id;

    mysock->send("DDP DL GET " + s.str() + " " + var);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    return answer;
}


// target PKG
std::vector<package> downloadc::get_packages(){
    check_connection();

    std::string answer;

    skip_update = true;
    mx.lock();
    mysock->send("DDP DL LIST");
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);
    mx.unlock();
    skip_update = false;

    std::vector<std::vector<std::string> > new_content;
    this->split_special_string(new_content, answer);

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

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    mysock->send("DDP PKG ADD " + name);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;

    int id = atoi(answer.c_str());
    if(id == -1){
        throw client_exception(9, "failed to create package");
    }
    return id;
}


void downloadc::delete_package(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG DEL " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
}


void downloadc::package_priority_up(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG UP " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
}


void downloadc::package_priority_down(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG DOWN " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
}


bool downloadc::package_exists(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP PKG EXISTS " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;

    if(answer == "0")
        return false;

    return true;
}


void downloadc::set_package_var(int id, std::string var, std::string value){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;
    std::stringstream s;
    s << id;

    mysock->send("DDP PKG SET " + s.str() + " " + var + " = " + value);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


std::string downloadc::get_package_var(int id, std::string var){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;
    std::stringstream s;
    s << id;

    mysock->send("DDP PKG GET " + s.str() + " " + var);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    return answer;
}


void downloadc::pkg_container(std::string type, std::string content){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    mysock->send("DDP PKG CONTAINER " + type + ":" + content);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


// target VAR
void downloadc::set_var(std::string var, std::string value, std::string old_value ){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    if(var == "mgmt_password")
        value = old_value + " ; " + value;

    mysock->send("DDP VAR SET " + var + " = " + value);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


std::string downloadc::get_var(std::string var){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);
    std::string answer;

    mysock->send("DDP VAR GET " + var);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    return answer;
}


// target FILE
void downloadc::delete_file(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP FILE GETPATH " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    if(answer.empty()) // no error if file doesn't exist
        return;

    mysock->send("DDP DL DEACTIVATE " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    mysock->send("DDP FILE DEL " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


std::string downloadc::get_file_path(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP FILE GETPATH " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    return answer;
}


uint64_t downloadc::get_file_size(int id){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;
    std::stringstream id_str;
    id_str << id;

    mysock->send("DDP FILE GETSIZE " + id_str.str());
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;

    return atol(answer.c_str());
}


// target ROUTER
std::vector<std::string> downloadc::get_router_list(){
    check_connection();

    std::string model_list;

    skip_update = true;
    mx.lock();
    mysock->send("DDP ROUTER LIST");
    mysock->recv(model_list);
    while(!check_correct_answer(model_list))
        mysock->recv(model_list);
    mx.unlock();
    skip_update = false;

    return split_string(model_list, "\n");
}


void downloadc::set_router_model(std::string model){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP ROUTER SETMODEL " + model);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::set_router_var(std::string var, std::string value){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP ROUTER SET " + var + " = " + value);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


std::string downloadc::get_router_var(std::string var){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP ROUTER GET " + var);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    return answer;
}


// target PREMIUM
std::vector<std::string> downloadc::get_premium_list(){
    check_connection();

    std::string host_list;

    skip_update = true;
    mx.lock();
    mysock->send("DDP PREMIUM LIST");
    mysock->recv(host_list);
    while(!check_correct_answer(host_list))
        mysock->recv(host_list);
    mx.unlock();
    skip_update = false;

    return this->split_string(host_list, "\n");
}


void downloadc::set_premium_var(std::string host, std::string user, std::string password){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP PREMIUM SET " + host + " " + user + ";" + password);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


std::string downloadc::get_premium_var(std::string host){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer;

    mysock->send("DDP PREMIUM GET " + host);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    return answer;
}


std::vector<std::string> downloadc::get_subscription_list(){
    check_connection();

    std::string sub_list;

    skip_update = true;
    mx.lock();
    mysock->send("DDP SUBSCRIPTION LIST");
    mysock->recv(sub_list);
    while(!check_correct_answer(sub_list))
        mysock->recv(sub_list);
    mx.unlock();
    skip_update = false;

    return split_string(sub_list, "\n");
}


void downloadc::add_subscription(subs_type type){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer, t_subs;
    subs_to_string(type, t_subs);

    mysock->send("DDP SUBSCRIPTION ADD " + t_subs);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


void downloadc::remove_subscription(subs_type type){
    check_connection();

    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    std::string answer, t_subs;
    subs_to_string(type, t_subs);

    mysock->send("DDP SUBSCRIPTION DEL " + t_subs);
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;
    check_error_code(answer);
}


// helper functions
void downloadc::check_connection(){
    skip_update = true;
    std::lock_guard<std::mutex> lock(mx);

    if(mysock == NULL){ // deleted mysock
        skip_update = false;
        throw client_exception(10, "connection lost");
    }

    std::string answer;

    mysock->send("hello :3"); // send something, so we can test the connection
    mysock->recv(answer);
    while(!check_correct_answer(answer))
        mysock->recv(answer);

    skip_update = false;

    if(!*mysock){ // if there is no active connection
        throw client_exception(10, "connection lost");
    }
}


std::vector<std::string> downloadc::split_string(const std::string& inp_string, const std::string& seperator, bool respect_escape){
    std::vector<std::string> ret;
        size_t n = 0, last_n = 0;
        while(true){
            n = inp_string.find(seperator, n);

            if(respect_escape && (n != std::string::npos) && (n != 0)){
                if(inp_string[n-1] == '\\') {
                    ++n;
                    continue;
                }
            }

        ret.push_back(inp_string.substr(last_n, n - last_n));
        size_t esc_pos = 0;

        if(respect_escape){
            while((esc_pos = ret.back().find('\\' + seperator)) != std::string::npos)
                ret.back().erase(esc_pos, 1);
        }

        if(n == std::string::npos)
            break;
        n += seperator.size();
        last_n = n;

    }

    return ret;
}


const std::string &downloadc::trim_string(std::string &str){
    while(str.length() > 0 && isspace(str[0])){
        str.erase(str.begin());
    }

    while(str.length() > 0 && isspace(*(str.end() - 1))){
        str.erase(str.end() -1);
    }

    return str;
}


void downloadc::split_special_string(std::vector<std::vector<std::string> > &new_content, std::string &answer){
    std::vector<std::string> splitted_line;
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


bool downloadc::check_correct_answer(std::string check_me){
// returns true if check_me is a real answer and not a subscription string
// if check_me is a subscription string, it will get stored for later use
    bool correct = true;

    if(check_me.find("SUBS") == 0){ // it's a subscription string!
        old_updates.push_back(check_me); // store check_me
        correct = false;
    }

    return correct;
}


void downloadc::subs_to_string(subs_type t, std::string &ret){
    switch(t){
        case SUBS_DOWNLOADS:
            ret = "SUBS_DOWNLOADS";
            return;
        case SUBS_CONFIG:
            ret = "SUBS_CONFIG";
            return;
        case SUBS_NONE:
            ret = "";
            return;
    }
}
