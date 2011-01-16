#ifndef DDAPI_H
#define DDAPI_H

#include "angelscript.h"
#include "scriptany.h"
#include <ddcurl.h>
#include <string>
#include <map>

class ddapi;

struct chandle_info {
	chandle_info();
//		: write_data(ddapi::instance->engine), progress_data(ddapi::instance->engine), header_data(ddapi::instance->engine) {}
	ddcurl *handle;
	std::string write_callback;
	CScriptAny write_data;
	std::string progress_callback;
	CScriptAny progress_data;
	std::string header_callback;
	CScriptAny header_data;
private:
	virtual ~chandle_info() {}
};

typedef std::map<ui32, chandle_info> handlemap;

class ddapi {
	friend struct chandle_info;
public:
	static void start_engine();
	static void shutdown();
	static void exec(const std::string& command);
	static ui32 add_handle(ddcurl &h);
	static void remove_handle(ui32 id);

	static ui32 new_handle();
	static void delete_handle(ui32 id);

	static chandle_info* get_handle(ui32 id);

	static CURLcode setopt_wrap(ui32 handle, CURLoption opt, void *value, int type);
	static CURLcode perform_wrap(ui32 id);

	static bool script_func_exists(const std::string &name);

private:
	ddapi();
	ddapi(const ddapi &);
	static ddapi *instance;

	asIScriptEngine *engine;

	handlemap chandles;

	static int progress_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static int header_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static size_t write_cb( void *ptr, size_t size, size_t nmemb, void *userdata);

};

// RegisterDDApi...(asIScriptEngine *engine), maybe a class to access plugin-details, etc


#endif
