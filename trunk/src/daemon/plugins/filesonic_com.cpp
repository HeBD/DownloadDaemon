/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <cctype>
#include <string>
using namespace std;


bool is_number(const std::string& s)
{
	for (int i = 0; i < s.length(); i++) {
		if (!std::isdigit(s[i]))
			return false;
	}
	return true;
}


size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

//Only Premium-User support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	//dcurl* fileexists = get_handle();
	//ddcurl* get_domain = get_handle();
	string url = get_url();
	string result;
	result.clear();
	if(inp.premium_user.empty() || inp.premium_password.empty()) {
		return PLUGIN_AUTH_FAIL; // free download not supported yet
	}

	//encode login data
	string premium_user = handle->escape(inp.premium_user);
	string premium_pwd = handle->escape(inp.premium_password);
	
	vector<string> splitted_url = split_string(url, "/");
	/*//get domain for filesonic
	get_domain->setopt(CURLOPT_URL, "http://api.filesonic.com/utility?method=getFilesonicDomainForCurrentIp");
	get_domain->setopt(CURLOPT_HTTPPOST, 0);
	get_domain->setopt(CURLOPT_WRITEFUNCTION, write_data);
	get_domain->setopt(CURLOPT_WRITEDATA, &result);
	int ret = get_domain->perform();
	string domain = search_between(result, "\"response\":\"","\"");
	//log_string("filesonic.com: domain" + domain, LOG_DEBUG);

	
	splitted_url[2]= "www" + domain;
	url.clear();
	for(size_t i = 0; i < splitted_url.size(); ++i) {
	   url += splitted_url[i]+"/";}
	//log_string("filesonic.com: url" + url, LOG_DEBUG);
	result.clear();

	//is file deleted?
	fileexists->setopt(CURLOPT_URL, url.c_str());
	fileexists->setopt(CURLOPT_HTTPPOST, 0);
	fileexists->setopt(CURLOPT_WRITEFUNCTION, write_data);
	fileexists->setopt(CURLOPT_WRITEDATA, &result);
	ret = fileexists->perform();
	if (result.find("This file was deleted") != std::string::npos)
		return PLUGIN_FILE_NOT_FOUND;
	//log_string("filesonic.com: result=" + result, LOG_DEBUG);
	//Setup post data for Premium Account
	result.clear();
	//string data = "redirect=&email="+premium_user+"&password="+premium_pwd+"&rememberMe=0&controls%5Bsubmit%5D=";*/
	string id;
	if(is_number(splitted_url[4]))
		id = splitted_url[4];
	else if(is_number(splitted_url[5]))
		id = splitted_url[5];
	else
	{
		log_string("filesonic.com: cannot find id",LOG_DEBUG);
		return PLUGIN_ERROR;
	}
	url = "http://api.filesonic.com/link?method=getDownloadLink&u=" + premium_user + "&p=" + premium_pwd + "&format=xml&ids=" + id;
	handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
	handle->setopt(CURLOPT_URL, url.c_str());
	handle->setopt(CURLOPT_HTTPPOST, 0);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	int ret = handle->perform();
	//log_string("filesonic.com: result=" + result, LOG_DEBUG);
	if(ret != CURLE_OK)
		return PLUGIN_CONNECTION_ERROR;
	if(ret == 0) {
		if (result.find("FSApi_Auth_Exception")!=string::npos)
		{
			return PLUGIN_AUTH_FAIL;
		}
		string status = search_between(result,"<status>","</status");
		if (status == "NOT_AVAILABLE")
		{
			return PLUGIN_FILE_NOT_FOUND;
		}
		string url = search_between(result,"<url><![CDATA[","]]></url>");
		outp.download_url = url.c_str();
		//log_string("url = " + url,LOG_DEBUG);
		return PLUGIN_SUCCESS;
	}
	return PLUGIN_ERROR;

}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
		outp.allows_multiple = false;
	}
	outp.offers_premium = true;
}
