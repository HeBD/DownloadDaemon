#ifndef DDCURL_H
#define DDCURL_H

#include <config.h>
#include <curl/curl.h>
#include <string>

/*! This class is a simple libcurl-wrapper that adds the possibility to use the ddproxy.php script in a simple way */
class ddcurl {
public:
	ddcurl(bool do_init = false) : handle(0) {
		if(do_init) handle = curl_easy_init();
	}

	~ddcurl() {
		if(handle) cleanup();
	}

	void init() {
		if(handle) cleanup();
		handle = curl_easy_init();
	}

	void cleanup() {
		if(handle) curl_easy_cleanup(handle);
		handle = 0;
		m_sProxy.clear();
		m_sProxyUser.clear();
		m_sProxyPass.clear();
		m_sUrl.clear();
	}

	template <typename T>
	CURLcode setopt(CURLoption opt, const T &val) {
		if(!handle) handle = curl_easy_init();
		return curl_easy_setopt(handle, opt, val);
	}

	CURLcode setopt(CURLoption opt, const std::string &val) {
		if(!handle) handle = curl_easy_init();
		if(opt == CURLOPT_URL) { // the URL might have to be modified in the future because of ddproxy-settings.
			// so we only save it and apply the URL to the curl handle just before perform()ing.
			m_sUrl = val;
			return CURLE_OK;
		}
		if(opt != CURLOPT_PROXY && opt != CURLOPT_PROXYUSERPWD && opt != CURLOPT_PROXYUSERNAME && opt != CURLOPT_PROXYPASSWORD)
			return curl_easy_setopt(handle, opt, val.c_str());

		size_t sep_pos;
		switch(opt) {
		case CURLOPT_PROXY:
			if((val.find("http://") == 0 || val.find("https://") == 0) && val.rfind(".php") == val.size() - 4) {
				m_sProxy = val;
				return CURLE_OK;
			} else {
				m_sProxy.clear();
				return curl_easy_setopt(handle, opt, val.c_str());
			}
			break;
		case CURLOPT_PROXYUSERPWD:
			sep_pos = val.find(":");
			m_sProxyUser = val.substr(0,sep_pos);
			if(val.size() > sep_pos)
				m_sProxyPass = val.substr(sep_pos + 1);
			return CURLE_OK;
		case CURLOPT_PROXYUSERNAME:
			m_sProxyUser = val;
			return CURLE_OK;
		case CURLOPT_PROXYPASSWORD:
			m_sProxyPass = val;
			return CURLE_OK;
		default:
			curl_easy_setopt(handle, opt, val.c_str());
		}


		return CURLE_OK;
	}

	CURLcode setopt(CURLoption opt, const char *val) {
		return setopt(opt, std::string(val));
	}

	CURLcode perform() {
		if (m_sUrl.empty()) return CURLE_URL_MALFORMAT;
		std::string url = m_sUrl;
		if(!m_sProxy.empty()) {
			url = m_sProxy + "?ddproxy_pass=" + escape(m_sProxyPass) + "&ddproxy_url=" + escape(m_sUrl);
		} else {
			curl_easy_setopt(handle, CURLOPT_PROXYUSERNAME, m_sProxyUser.c_str());
			curl_easy_setopt(handle, CURLOPT_PROXYPASSWORD, m_sProxyPass.c_str());

		}
		curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
		return curl_easy_perform(handle);
	}

	template <typename T>
		CURLcode getinfo(CURLINFO info, T val) const {
		return curl_easy_getinfo(handle, info, val);
	}


	static std::string escape(const std::string &url) {
		CURL *tmph = curl_easy_init();
		char *ret = curl_easy_escape(tmph, url.c_str(), url.size());
		curl_easy_cleanup(tmph);
		if(!ret) return url;
		std::string tmp(ret);
		curl_free(ret);
		return tmp;
	}

	static std::string unescape(const std::string &url) {
		CURL *tmph = curl_easy_init();
		int olen;
		char *ret = curl_easy_unescape(tmph, url.c_str(), url.size(), &olen);
		curl_easy_cleanup(tmph);
		if(!ret) return url;
		std::string tmp(ret, olen);
		curl_free(ret);
		return tmp;
	}

	void reset() {
		if(!handle) handle = curl_easy_init();
		else curl_easy_reset(handle);
		m_sProxy.clear();
		m_sProxyUser.clear();
		m_sProxyPass.clear();
		m_sUrl.clear();
	}

	CURL* raw_handle() {
		return handle;
	}

private:
	ddcurl(const ddcurl &c); // private copy constructor

	CURL *handle;
	std::string m_sProxy;
	std::string m_sProxyUser;
	std::string m_sProxyPass;

	std::string m_sUrl;
};

#endif
