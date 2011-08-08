/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_CAN_PRECHECK
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) 
{
        std::string *blubb = (std::string*)userp;
        blubb->append((char*)buffer, nmemb);
        return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
        ddcurl* handle = get_handle();
        string result;
        handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_HEADER, 1);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
        handle->setopt(CURLOPT_COOKIEFILE, "");
        int res = handle->perform();
        if(res != 0)
        {
                return PLUGIN_CONNECTION_ERROR;
        }
	string newurl;
	download_container urls;
	//download_container* dlc = get_dl_container();
	//download *download = get_dl_ptr();
	//int dlid = download->get_id();
        try {
                //file deleted?
                if(result.find("(\"This link does not exist.\"|ERROR - this link does not exist)")!= std::string::npos)
                    return PLUGIN_FILE_NOT_FOUND;
                if(result.find(">Not yet checked</span>")!= std::string::npos)
                    return PLUGIN_ERROR;
                if(result.find("To use reCAPTCHA you must get an API key from")!= std::string::npos)
                {
                    set_wait_time(600);
                    return PLUGIN_SERVER_OVERLOADED;
                }
                string url = get_url();
                if(url.find("/d/")== std::string::npos)
                {
                    string rslt="";
                    string post = "post-protect=1";
                    handle->setopt(CURLOPT_HEADER, 0);
                    //because sometimes the first time it doesnt work
                    for (int i = 0; i <= 5; i++)
                    {
                        if(result.find("type=\"password\" name=\"link-password\"")!= std::string::npos)
                        {
                            //password protected => ask user for password
                            std::string captcha_text = Captcha.process_image(rslt, "", "", -1, false, false, captcha::SOLVE_MANUAL);
                            post += "&link-password=" + captcha_text;

                        }
                        if(result.find("api.recaptcha.net")!= std::string::npos)
                        {
                            //recaptcha!!!
                            size_t urlpos = result.find("<iframe src=\"http://api.recaptcha.net/noscript?k=");
                            if(urlpos == string::npos) return PLUGIN_ERROR;
                            urlpos += 49;
                            string id = result.substr(urlpos, result.find("\"", urlpos) - urlpos - 12); //&amp;error=1"
                            log_string("Safelinking plugin: id = : " + id, LOG_DEBUG);
                            if(id == "")
                            {
                                return PLUGIN_ERROR;
                            }
                            else
                            {
                                handle->setopt(CURLOPT_URL, "http://api.recaptcha.net/challenge?k=" + id);
                                /* follow redirect needed as google redirects to another domain */
                                handle->setopt(CURLOPT_FOLLOWLOCATION,1);
                                result.clear();
                                handle->perform();
                                urlpos = result.find("challenge : '");
                                if(urlpos == string::npos) return PLUGIN_ERROR;
                                urlpos += 13;
                                string challenge = result.substr(urlpos, result.find("',", urlpos) - urlpos);
                                urlpos = result.find("server : '");
                                if(urlpos == string::npos) return PLUGIN_ERROR;
                                urlpos += 9;
                                string server = result.substr(urlpos, result.find("',", urlpos) - urlpos);
                                if(challenge == "" || server == "")
                                    return PLUGIN_ERROR;
                                string captchaAddress = server + "image?c=" + challenge;
                                std::string captcha_text = Captcha.process_image(captchaAddress, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
                                post += "&recaptcha_challenge_field=" + challenge;
                                post += "&recaptcha_response_field=" + captcha_text;
                            }
                        }
                        else if(size_t urlpos=result.find("http://safelinking\\.net/includes/captcha_factory/securimage/securimage_show\\.php\\?sid=")!= std::string::npos)
                        {
                            string captchaAddress = result.substr(urlpos, result.find("\"", urlpos) - urlpos);
                            std::string captcha_text = Captcha.process_image(captchaAddress, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
                            post+= "&securimage_response_field=" + captcha_text;
                        }
                        else if(size_t urlpos=result.find("http://safelinking\\.net/includes/captcha_factory/3dcaptcha/3DCaptcha\\.php")!= std::string::npos)
                        {
                            string captchaAddress = result.substr(urlpos, result.find("\"", urlpos) - urlpos);
                            std::string captcha_text = Captcha.process_image(captchaAddress, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
                            post+= "&3dcaptcha_response_field=" + captcha_text;
                        }
                        else if(size_t urlpos=result.find("fancycaptcha\\.css\"")!= std::string::npos)
                        {
                            handle->setopt(CURLOPT_URL, "http://safelinking.net/includes/captcha_factory/fancycaptcha.php");
                            result.clear();
                            handle->perform();
                            std::remove(result.begin(), result.end(), ' ');
                            post+= "&fancy-captcha=" + result;
                        }
                        handle->setopt(CURLOPT_COPYPOSTFIELDS, post);
                        handle->setopt(CURLOPT_URL, get_url());
                        handle->setopt(CURLOPT_POST, 1);
                        result.clear();
                        handle->perform();
                        if (result.find("api.recaptcha.net")!= std::string::npos ||
                            result.find("http://safelinking\\.net/includes/captcha_factory/securimage/securimage_show\\.php\\?sid=")!= std::string::npos ||
                            result.find("http://safelinking\\.net/includes/captcha_factory/3dcaptcha/3DCaptcha\\.php")!= std::string::npos ||
                            result.find("type=\"password\" name=\"link-password\"")!= std::string::npos)
                        {
                            continue;
                        }
                        if(result.find("fancycaptcha\\.css\"")!= std::string::npos)
                        {
                            log_string("Safelinking plugin failed with fancycaptcha = " + url, LOG_DEBUG);
                            return PLUGIN_ERROR;
                        }
                        break;
                    }
                    if (result.find("api.recaptcha.net")!= std::string::npos ||
                        result.find("http://safelinking\\.net/includes/captcha_factory/securimage/securimage_show\\.php\\?sid=")!= std::string::npos ||
                        result.find("http://safelinking\\.net/includes/captcha_factory/3dcaptcha/3DCaptcha\\.php")!= std::string::npos ||
                        result.find("type=\"password\" name=\"link-password\"")!= std::string::npos)
                    {
                        log_string("Safelinking plugin failed after 6 times = " + url, LOG_DEBUG);
                        return PLUGIN_ERROR;
                    }
                    if(result.find(">All links are dead\\.<")!= std::string::npos)
                    {
                        return PLUGIN_FILE_NOT_FOUND;
                    }
                    /*to do=>add links to download + testing plugin!*/
                }
                else
                {
                    //just a simple redirect
                    size_t urlpos = result.find("Location:");
                    if(urlpos == string::npos) return PLUGIN_ERROR;
                    urlpos += 10;
                    newurl = result.substr(urlpos, result.find("\n", urlpos) - urlpos);
                    urls.add_download(set_correct_url(newurl),"");
                }
        } catch(...) {}
	replace_this_download(urls);
	//dlc->add_download(set_correct_url(newurl), "");
	//dlc->set_status(dlid, DOWNLOAD_DELETED);
        //set_url(set_correct_url(newurl));
        return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) 
{
        return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) 
{
        outp.allows_resumption = false;
        outp.allows_multiple = false;

        outp.offers_premium = false;
}
