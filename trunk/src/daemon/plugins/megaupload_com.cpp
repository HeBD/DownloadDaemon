/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_WANTS_POST_PROCESSING
#define PLUGIN_CAN_PRECHECK
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

bool free_member = false;
bool premium_member = false;

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	string result;
	string url = get_url();
	if(url.find("/?f=")== std::string::npos) {
		if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
			handle->setopt(CURLOPT_COOKIEFILE, "");
			handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			handle->setopt(CURLOPT_URL, "http://www.megaupload.com/?c=login&setlang=en");
			handle->setopt(CURLOPT_POST, 1);
			string to_post = "login=1&redir=1&username=" + inp.premium_user + "&password=" + inp.premium_password;
			handle->setopt(CURLOPT_COPYPOSTFIELDS, to_post.c_str());
			int res = handle->perform();
			if(res != 0) {
				return PLUGIN_CONNECTION_ERROR;
			}

			handle->setopt(CURLOPT_POST, 0);
			if(result.find("Username and password do not match") != string::npos)
				return PLUGIN_AUTH_FAIL;

			if (premium_member) {
				outp.download_url = get_url();
				return PLUGIN_SUCCESS;
			}
		}
		bool done = false;
		// so we can never get in an infinite loop..
		int while_tries = 0;
		while(!done && while_tries < 100) {
			++while_tries;
			handle->setopt(CURLOPT_URL, get_url());
			handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle->setopt(CURLOPT_WRITEDATA, &result);
			handle->setopt(CURLOPT_COOKIEFILE, "");
			handle->setopt(CURLOPT_FOLLOWLOCATION, 1);
			result.clear();
			int res = handle->perform();
			if(res != 0) {
				return PLUGIN_CONNECTION_ERROR;
			}
			//log_string("megaupload.com:" + result,LOG_DEBUG);

			if(result.find("Unfortunately, the link you have clicked is not available") != string::npos) {
				return PLUGIN_FILE_NOT_FOUND;
			}

			if(result.find("The file that you're trying to download is larger than 1 GB") != string::npos
			   || result.find("The file you're trying to download is password protected") != string::npos) {
				return PLUGIN_AUTH_FAIL;
			}

			if(result.find("We have detected an elevated number of requests from your IP address. You have to wait 2 minutes") != string::npos) {
				set_wait_time(120);
				return PLUGIN_SERVER_OVERLOADED;
			}

			if(result.find("gencap.php") == string::npos) {
				// as of dec 2010, they don't want a captcha any more.
				// I don't know if that's an exception, but I'll just leave the captcha-code here and ignore it if not needed
				int pos = result.find("class=\"download_premium_but\"></a>");
				string url = search_between(result, "<a href=\"", "\"",pos);
				result.clear();
				//set up handle to only get header
				handle->setopt(CURLOPT_URL,url.c_str());
				handle->setopt(CURLOPT_HEADER, true); 
				handle->setopt(CURLOPT_NOBODY, true); 
				handle->setopt(CURLOPT_FORBID_REUSE, false); 
				//handle->setopt(CURLOPT_RETURNTRANSFER, true);
				int res = handle->perform();
				//log_string("megaupload.com: " + result,LOG_DEBUG);
				pos = result.find("HTTP/1.1 ");
				pos+=9;
				int responscode = atoi(result.substr(pos,3).c_str());
				if(responscode==503)
				{
					set_wait_time(600);
					return PLUGIN_LIMIT_REACHED;
				}
				else if(responscode==416)
				{
					set_wait_time(600);
					return PLUGIN_SERVER_OVERLOADED;
				}
				else if(responscode==404)
				{
					return PLUGIN_FILE_NOT_FOUND;
				}
				else if(responscode==200)
				{
					outp.download_url = url;
				}
				else
				{
					return PLUGIN_ERROR;
				}
				//set_wait_time(atoi(search_between(result, "count=", ";").c_str()));
				handle->setopt(CURLOPT_HEADER, false); 
				handle->setopt(CURLOPT_NOBODY, false); 
				handle->setopt(CURLOPT_FORBID_REUSE, true); 
				if(outp.download_url.empty())
					return PLUGIN_FILE_NOT_FOUND;
				return PLUGIN_SUCCESS;
			}

			string captcha_url;
			try {
				size_t n = result.find("<TD width=\"100\" align=\"center\" height=\"40\"><img src=\"");
				n = result.find("http://", n);
				captcha_url = result.substr(n, result.find("\"", n) - n);

				n = result.find("name=\"captchacode\" value=\"") + 26;
				std::string captchacode = result.substr(n, result.find("\"", n) - n);

				n = result.find("name=\"megavar\"");
				n = result.find("value=\"", n) + 7;
				std::string megavar = result.substr(n, result.find("\"", n) - n);


				handle->setopt(CURLOPT_URL, captcha_url.c_str());
				result.clear();
				res = handle->perform();
				if(res != 0) {
					return PLUGIN_ERROR;
				}
				// megaupload uses .gif captchas that don't work with gocr. We use giftopnm to convert it (which is installed with netpbm, which
				// is a gocr dependency anyway.
				ofstream captcha_fs("/tmp/tmp_captcha_megaupload.gif");
				captcha_fs << result;
				captcha_fs.close();
				FILE* captcha_as_pnm = popen("giftopnm /tmp/tmp_captcha_megaupload.gif", "r");

				if(captcha_as_pnm == NULL) {
					captcha_exception e;
					throw e;
				}
				// if the captcha is larger than 10kb.. it's not.
				result.clear();

				int c;
				do{
					c = fgetc(captcha_as_pnm);
					if(c != EOF) result.push_back(c);
				} while(c != EOF);


				pclose(captcha_as_pnm);
				remove("/tmp/tmp_captcha_megaupload.gif");;
				std::string captcha_text = Captcha.process_image(result, "pnm", "-C 1-9A-NP-Z", 4, true);
				if(captcha_text.empty()) continue;

				std::string post_data = "captchacode=" + captchacode + "&megavar=" + megavar + "&captcha=" + captcha_text;

				handle->setopt(CURLOPT_URL, get_url());
				handle->setopt(CURLOPT_POST, 1);
				handle->setopt(CURLOPT_COPYPOSTFIELDS, post_data.c_str());
				result.clear();
				res = handle->perform();
				handle->setopt(CURLOPT_POST, 0);
				if(res != 0) {
					return PLUGIN_ERROR;
				}
				if(result.find("<input type=\"text\" name=\"captcha\" id=\"captchafield\"") != string::npos) {
					// captcha wrong.. try again
					continue;
				} else {
					set_wait_time(45);
					n = result.find("id=\"downloadlink\"><a href=\"") + 27;
					outp.download_url = result.substr(n, result.find("\"", n) - n);
					return PLUGIN_SUCCESS;

				}

			} catch(std::exception &e) {
				return PLUGIN_ERROR;
			}
		}
	}
	else
	{
		//megaupload folder
		download_container urls;
		size_t n = url.find("/?f=");
		n += 4;
		string id = url.substr(n);
		//log_string("Megaupload.com: id=" + id,LOG_DEBUG);
		string newurl = "http://www.megaupload.com/xml/folderfiles.php?folderid=" + id;
		//log_string("Megaupload.com: url=" + newurl,LOG_DEBUG);
		handle->setopt(CURLOPT_URL, newurl.c_str());
		handle->setopt(CURLOPT_POST, 0);
		handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
		handle->setopt(CURLOPT_WRITEDATA, &result);
		handle->setopt(CURLOPT_COOKIEFILE, "");
		result.clear();
		int res = handle->perform();
		if(res != 0)
		{
			return PLUGIN_CONNECTION_ERROR;
		}
		//log_string("Megaupload.com: result=" + result,LOG_DEBUG);
		vector<string> links = split_string(result, "</ROW>");
		for(size_t i = 0; i < links.size()-1; i++)
		{
			size_t urlpos = links[i].find("url=\"");
			if(urlpos == string::npos)
			{
				log_string("Megaupload.com: urlpos is at end of file",LOG_DEBUG);
				return PLUGIN_ERROR;
			}
			urlpos += 5;
			string temp = links[i].substr(urlpos, links[i].find("\"", urlpos) - urlpos);
			//log_string("Megaupload.com: link=" + temp,LOG_DEBUG);
			urls.add_download(temp, "");
		}
	replace_this_download(urls);
	}
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) {
	std::string url = get_url();
	if(url.find("/?f=")!= std::string::npos)
		return false;
	std::string result;
	ddcurl handle;
	handle.setopt(CURLOPT_LOW_SPEED_LIMIT, (long)10);
	handle.setopt(CURLOPT_LOW_SPEED_TIME, (long)20);
	handle.setopt(CURLOPT_CONNECTTIMEOUT, (long)30);
	handle.setopt(CURLOPT_NOSIGNAL, 1);
	handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle.setopt(CURLOPT_WRITEDATA, &result);
	handle.setopt(CURLOPT_COOKIEFILE, "");
	handle.setopt(CURLOPT_FOLLOWLOCATION, true);
	handle.setopt(CURLOPT_URL, url.c_str());
	int res = handle.perform();
	handle.cleanup();
	if(res != 0) {
		outp.file_online = PLUGIN_CONNECTION_LOST;
		return true;
	}
	try {
		outp.download_filename = search_between(result, "<span class=\"down_txt2\">", "</span>");
		if(outp.download_filename.empty()) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		size_t n = result.find("File size:");
		if(n == string::npos) {
			outp.file_online = PLUGIN_FILE_NOT_FOUND;
			return true;
		}
		string fs_str = search_between(result, "</strong> ", "<br", n);
		vector<string> value = split_string(fs_str, " ");

		const char * oldlocale = setlocale(LC_NUMERIC, "C");

		size_t size = 0;
		if(value[1].find("KB") != string::npos)
			size = strtod(value[0].c_str(), NULL) * 1024;
		else if(value[1].find("M") != string::npos)
			size = strtod(value[0].c_str(), NULL) * (1024*1024);

		setlocale(LC_NUMERIC, oldlocale);

		outp.file_size = size;
	} catch(...) {
		outp.file_online = PLUGIN_FILE_NOT_FOUND;
	}
	return true;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if (premium_member) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	}
	if (free_member) {
		outp.allows_resumption = true;
		outp.allows_multiple = false;
	}
	if (!premium_member && !free_member) {
		if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
			outp.allows_resumption = true;
			ddcurl handle;
			string result;
			handle.setopt(CURLOPT_COOKIEFILE, "");
			handle.setopt(CURLOPT_FOLLOWLOCATION, 0);
			handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
			handle.setopt(CURLOPT_WRITEDATA, &result);
			handle.setopt(CURLOPT_URL, "http://www.megaupload.com/?c=login&setlang=en");
			handle.setopt(CURLOPT_POST, 1);
			string to_post = "login=1&redir=1&username=" + inp.premium_user + "&password=" + inp.premium_password;
			handle.setopt(CURLOPT_COPYPOSTFIELDS, to_post.c_str());
			handle.perform();

			handle.setopt(CURLOPT_POST, 0);
			if(result.find("Username and password do not match") != string::npos) {
				outp.allows_resumption = true;
				outp.allows_multiple = false;
				premium_member = false;
				free_member = false;
			}
			if(result.find("Username and password do not match") == string::npos) {
				//receive account details
				result.clear();
				handle.setopt(CURLOPT_WRITEFUNCTION, write_data);
				handle.setopt(CURLOPT_WRITEDATA, &result);
				handle.setopt(CURLOPT_URL, "http://www.megaupload.com/?c=account");
				handle.perform();

				if(result.find("Regular") != string::npos) {
					outp.allows_multiple = false;
					premium_member = false;
					free_member = true;
					log_string("Megaupload.com: Using free Member Account", LOG_DEBUG);
				}
				else {
					outp.allows_multiple = true;
					premium_member = true;
					free_member = false;
				}
			}
		}
		else {
			outp.allows_resumption = true;
			outp.allows_multiple = false;
		}
	}
	outp.offers_premium = true;
}
void post_process_download(plugin_input &inp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		return;
	} else {
		string filename = dl_list->get_output_file(dlid);
		struct pstat st;
		if(pstat(filename.c_str(), &st) != 0) {
			return;
		}
		if(st.st_size > 1024 * 1024 /* 1 mb */ ) {
			return;
		}
		ifstream ifs(filename.c_str());
		string tmp;
		getline(ifs, tmp);
		if(tmp.find("</HEAD><BODY>Download limit exceeded</BODY></HTML>") != string::npos) {
			set_wait_time(120);
			dl_list->set_status(dlid, DOWNLOAD_WAITING);
			dl_list->set_error(dlid, PLUGIN_LIMIT_REACHED);
		}


	}
}

