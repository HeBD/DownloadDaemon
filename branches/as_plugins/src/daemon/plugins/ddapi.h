#ifndef DDAPI_H
#define DDAPI_H

#include "angelscript.h"
#include "scriptany.h"
#include "../tools/helperfunctions.h"
#include <ddcurl.h>
#include <string>
#include <map>

struct chandle_info;
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

	static bool script_func_exists(const std::string &module, const std::string &name);

	static void reload_plugins();

	// this function can be overloaded with each number of arguments we need.
	template <typename T1, typename T2, typename T3>
	static T1 call_function(const std::string &module, const std::string &funcname, T2 &p1, T3 &p2) {
		asIScriptModule *mod = ddapi::instance->engine->GetModule(module.c_str());
		int func_id = mod->GetFunctionIdByName("plugin_getinfo");
		if(func_id < 0) {
			log_string(module + ": function " + funcname + " not found", LOG_ERR);
			return T1();
		}
		asIScriptContext *ctx = ddapi::instance->engine->CreateContext();
		ctx->Prepare(func_id);
		ctx->SetArgObject(0, &p1);
		ctx->SetArgObject(1, &p2);
		instance->curr_module = module;
		int r = ctx->Execute();
		instance->curr_module = "";
		if(r != asEXECUTION_FINISHED) {
			log_string("Could not execute " + module + "->" + funcname, LOG_ERR);
			return T1();
		}
		T1 ret;
		if(ctx->GetReturnAddress()) {
			ret = *((T1*)ctx->GetReturnAddress());
		} else if(ctx->GetReturnByte()) {
			ret = ctx->GetReturnByte();
		}
		ctx->Release();
		return ret;
	}

	std::string curr_module;

	~ddapi();

private:
	ddapi();
	ddapi(const ddapi &);
	static ddapi *instance;

	asIScriptEngine *engine;

	handlemap chandles;

	static int progress_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static int write_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static size_t header_cb( void *ptr, size_t size, size_t nmemb, void *userdata);

};

struct chandle_info {
	chandle_info()
		: write_data(new CScriptAny(ddapi::instance->engine)),
		  progress_data(new CScriptAny(ddapi::instance->engine)),
		  header_data(new CScriptAny(ddapi::instance->engine)) {}
	~chandle_info() {
		write_data->Release();
		progress_data->Release();
		header_data->Release();
	}

	ddcurl *handle;
	std::string write_callback;
	CScriptAny *write_data;
	std::string progress_callback;
	CScriptAny *progress_data;
	std::string header_callback;
	CScriptAny *header_data;
	std::string module;
};

// RegisterDDApi...(asIScriptEngine *engine), maybe a class to access plugin-details, etc


#endif
