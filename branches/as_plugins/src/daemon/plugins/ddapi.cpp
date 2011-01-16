#include "ddapi.h"
#include "angelscript.h"
#include "stdvector.h"
#include "scriptstdstring.h"
#include "scriptbuilder.h"
#include "scriptmath.h"
#include "scriptany.h"
#include "scriptarray.h"
#include "scriptdictionary.h"
#include "scripthelper.h"
#include "aswrappedcall.h"
#include "as_config.h"


#include "../tools/helperfunctions.h"
#include "../dl/download.h"
#include "../global.h"
#include <ddcurl.h>

#include <sstream>
#include <cassert>
#include <iostream>
#include <cstring>
#include <vector>

#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
using namespace std;

#ifdef AS_MAX_PORTABILITY
	#error DownloadDaemon is not able to detect your hardware correctly or doesnt support it. therefore it will not run.
	#error Feel free to make a post in the DownloadDaemon forums and explain the details of your hardware. A port is probably pretty simple.
#endif

ddapi* ddapi::instance = 0;


void as_msg_cb_log(const asSMessageInfo *msg, void *param) {
	int type = LOG_ERR;
	if( msg->type == asMSGTYPE_WARNING )
		type = LOG_WARNING;
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = LOG_INFO;
	ostringstream ss;
	ss << msg->section << ":" << msg->row << ":" << msg->col << " :: " << msg->message;
	log_string(ss.str(), type);
}

void as_msg_cb_cerr(const asSMessageInfo *msg, void *param) {
	string type = "ERROR";
	if(msg->type == asMSGTYPE_WARNING)
		type = "WARN";
	else if(msg->type == asMSGTYPE_INFORMATION)
		type = "INFO";
	cerr << type << ":" << msg->row << ":" << msg->col << " :: " << msg->message;
}

void ddapi::reload_plugins() {
	asIScriptEngine *e = ddapi::instance->engine;
	std::string plugindir = program_root + "plugins";
	correct_path(plugindir);

	DIR *dp;
	struct dirent *ep;
	dp = opendir(plugindir.c_str());
	if (dp == NULL) {
		log_string("Could not open plugin directory!", LOG_ERR);
		return;
	}

	struct pstat st;
	while ((ep = readdir(dp))) {
		if(ep->d_name[0] == '.') {
			continue;
		}
		string current = plugindir + '/' + ep->d_name;
		string name = ep->d_name;
		if(pstat(current.c_str(), &st) != 0) continue;

		if(current.rfind(".as") != current.size() - 3)
			continue;

		name.erase(name.size() - 3);
		e->DiscardModule(name.c_str());
		CScriptBuilder b;
		int r = b.StartNewModule(e, name.c_str()); assert(r>=0);
		r = b.AddSectionFromFile(current.c_str());
		int r2 = b.BuildModule();
		if(r < 0 || r2 < 0) {
			log_string("There appear to be errors in the " + name + " plugin. Please correct them", LOG_ERR);
			continue;
		}
	}
}

template <typename T>
void print(const T &val) {
	cout << val << endl;
}

template <typename T>
void printv(const vector<T> &v) {
	for(size_t i = 0; i < v.size(); ++i)
		cout << v[i] << endl;
}

template <typename T>
void typedef_inttype(asIScriptEngine *e, const string &type_name, bool _signed = true) {
	int r;
	string astype;
	if(!_signed)
		astype += "u";
	switch(sizeof(T)) {
	case 1:
		astype += "int8";
		break;
	case 2:
		astype += "int16";
		break;
	case 4:
		astype += "int";
		break;
	case 8:
		astype += "int64";
		break;
	default:
		assert(false);
	}
	r = e->RegisterTypedef(type_name.c_str(), astype.c_str()); assert(r>=0);
}

template <typename T>
string toa(T param) {
	ostringstream ss;
	ss << param;
	return ss.str();
}

int ddapi::progress_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	i32 id = *((i32*)clientp);
	chandle_info *i = ddapi::get_handle(id);
	if(!i || i->progress_callback.empty()) return 0;
	asIScriptContext *ctx = ddapi::instance->engine->CreateContext();
	int funcId = ddapi::instance->engine->GetModule(ddapi::instance->curr_module.c_str())->GetFunctionIdByName(i->progress_callback.c_str());
	ctx->Prepare(funcId);
	ctx->SetArgObject(0, i->progress_data);
	ctx->SetArgDouble(1, dltotal);
	ctx->SetArgDouble(2, dlnow);
	int r = ctx->Execute();
	i32 ret = 0;
	if(r == asEXECUTION_FINISHED) {
		ret = ctx->GetReturnDWord();
	}
	ctx->Release();
	return ret;
}

int ddapi::write_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	i32 id = *((i32*)clientp);
	chandle_info *i = ddapi::get_handle(id);
	if(!i || i->write_callback.empty()) return 0;
	asIScriptContext *ctx = ddapi::instance->engine->CreateContext();
	int funcId = ddapi::instance->engine->GetModule(ddapi::instance->curr_module.c_str())->GetFunctionIdByName(i->header_callback.c_str());
	ctx->Prepare(funcId);
	ctx->SetArgObject(0, i->write_data);
	ctx->SetArgDouble(1, dltotal);
	ctx->SetArgDouble(2, dlnow);
	int r = ctx->Execute();
	i32 ret = 0;
	if(r == asEXECUTION_FINISHED) {
		ret = ctx->GetReturnDWord();
	}
	ctx->Release();
	return ret;
}

size_t ddapi::header_cb( void *ptr, size_t size, size_t nmemb, void *userdata) {
	i32 id = *((i32*)userdata);
	chandle_info *i = ddapi::get_handle(id);
	if(!i || i->header_callback.empty()) return 0;
	asIScriptContext *ctx = ddapi::instance->engine->CreateContext();
	int funcId = ddapi::instance->engine->GetModule(ddapi::instance->curr_module.c_str())->GetFunctionIdByName(i->write_callback.c_str());
	ctx->Prepare(funcId);
	string data((const char*)ptr, nmemb * size);
	ctx->SetArgObject(0, &data);
	ctx->SetArgDouble(1, size);
	ctx->SetArgDouble(2, nmemb);
	ctx->SetArgObject(3, i->header_data);
	int r = ctx->Execute();
	size_t ret = 0;
	if(r == asEXECUTION_FINISHED) {
		if(sizeof(size_t) == 4)
			ret = ctx->GetReturnDWord();
		else
			ret = ctx->GetReturnQWord();
	}
	ctx->Release();
	return ret;
}

// for strings and callback-functions (which have to be passed as strings)
CURLcode ddapi::setopt_wrap(ui32 handle, CURLoption opt, void *value, int type) {
	asIScriptEngine *e = ddapi::instance->engine;
	chandle_info *i = ddapi::get_handle(handle);
	if(!i) return CURLE_FAILED_INIT;
	ddcurl *h = i->handle;
	if(opt == CURLOPT_POSTFIELDS)
		opt = CURLOPT_COPYPOSTFIELDS;

	if(type == e->GetTypeIdByDecl("int"))
		return h->setopt(opt, *((i32*)value));
	else if(type == e->GetTypeIdByDecl("uint"))
		return h->setopt(opt, *((ui32*)value));
	else if(type == e->GetTypeIdByDecl("int64"))
		return h->setopt(opt, *((i64*)value));
	else if(type == e->GetTypeIdByDecl("uint64"))
		return h->setopt(opt, *((ui64*)value));
	else if(type == e->GetTypeIdByDecl("string")) {
		string &s = *((string*)value);
		switch(opt) {
		case CURLOPT_PROGRESSFUNCTION:
			if(ddapi::script_func_exists(ddapi::instance->curr_module, s)) {
				i->progress_callback = s;
				return CURLE_OK;
			}
			return CURLE_FUNCTION_NOT_FOUND;
		case CURLOPT_WRITEFUNCTION:
			if(ddapi::script_func_exists(ddapi::instance->curr_module, s)) {
				i->write_callback = s;
				return CURLE_OK;
			}
			return CURLE_FUNCTION_NOT_FOUND;
		case CURLOPT_HEADERFUNCTION:
			if(ddapi::script_func_exists(ddapi::instance->curr_module, s)) {
				i->header_callback = s;
				return CURLE_OK;
			}
			return CURLE_FUNCTION_NOT_FOUND;
		default:
			return h->setopt(opt, s.c_str());
			break;
		}
	} else if(type == e->GetTypeIdByDecl("any")) {
		CScriptAny *a = (CScriptAny*)value;
		switch(opt) {
		case CURLOPT_PROGRESSDATA:
			i->progress_data->CopyFrom(a);
			return CURLE_OK;
		case CURLOPT_WRITEDATA:
			i->write_data->CopyFrom(a);
			return CURLE_OK;
		case CURLOPT_WRITEHEADER:
			i->header_data->CopyFrom(a);
			return CURLE_OK;
		default:
			log_string("ddapi::curl_setopt called with invalid argument type " + string(e->GetTypeDeclaration(type)), LOG_WARNING);
			break;
		}
	} else {
		log_string("ddapi::curl_setopt called with invalid argument type " + string(e->GetTypeDeclaration(type)), LOG_WARNING);
		return CURLE_FAILED_INIT;
	}

	return CURLE_FAILED_INIT;
}

asDECLARE_FUNCTION_WRAPPER(atof_wrap, atof);
asDECLARE_FUNCTION_WRAPPER(atol_wrap, atol);

ui32 ddapi::add_handle(ddcurl &h) {
	handlemap &m = ddapi::instance->chandles;
	ui32 max_id = 0;
	for(handlemap::iterator it = m.begin(); it != m.end(); ++it) {
		if(it->first > max_id)
			max_id = it->first;
	}
	++max_id;
	chandle_info info;
	info.handle = &h;
	m.insert(make_pair(max_id, info));
	return max_id;
}

CURLcode ddapi::perform_wrap(ui32 id) {
	chandle_info *i = ddapi::get_handle(id);
	if(!i) return CURLE_FAILED_INIT;
	ddcurl *h = i->handle;
	h->setopt(CURLOPT_PROGRESSFUNCTION, ddapi::progress_cb);
	h->setopt(CURLOPT_WRITEFUNCTION, ddapi::write_cb);
	h->setopt(CURLOPT_HEADERFUNCTION, ddapi::header_cb);
	h->setopt(CURLOPT_PROGRESSDATA, &id);
	h->setopt(CURLOPT_WRITEDATA, &id);
	h->setopt(CURLOPT_WRITEHEADER, &id);
	h->setopt(CURLOPT_NOPROGRESS, 0);
	return h->perform();
}

void ddapi::remove_handle(ui32 id) {
	handlemap::iterator it = ddapi::instance->chandles.find(id);
	if(it != ddapi::instance->chandles.end()) ddapi::instance->chandles.erase(it);
}

ui32 ddapi::new_handle() {
	ddcurl *h = new ddcurl(true);
	return ddapi::add_handle(*h);
}

void ddapi::delete_handle(ui32 id) {
	handlemap::iterator it = ddapi::instance->chandles.find(id);
	if(it != ddapi::instance->chandles.end()) {
		ddcurl *h = it->second.handle;
		ddapi::instance->chandles.erase(it);
		delete h;
	} else {
		log_string("delete_handle on nonexistant ddcurl object. This indicates a Plugin-bug", LOG_WARNING);
	}
}

chandle_info* ddapi::get_handle(ui32 id) {
	handlemap::iterator it = ddapi::instance->chandles.find(id);
	if(it != ddapi::instance->chandles.end()) return &(it->second);
	log_string("get_handle on nonexistant ddcurl object. This will probably cause problems.. Please report this", LOG_WARNING);
	return 0;
}

bool ddapi::script_func_exists(const std::string &module, const std::string &name) {
	// TODO
	return true;
}

ddapi::ddapi() {
	if(strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY")) {
		cerr << "DD will not run with AS_MAX_PORTABILITY" << endl;
		exit(-1);
	}
	int r;
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asFUNCTION(as_msg_cb_log), 0, asCALL_CDECL);
	RegisterStdString(engine);
	RegisterScriptMath(engine);
	RegisterScriptAny(engine);
	RegisterScriptArray(engine, true);
	RegisterScriptDictionary(engine);

	typedef_inttype<size_t>(engine, "size_t", false);
	typedef_inttype<time_t>(engine, "time_t", false);
	typedef_inttype<filesize_t>(engine, "filesize_t", false);

	// register vector types
	RegisterVector<int>("int[]", "int", engine);
	RegisterVector<double>("double[]", "double", engine);
	RegisterVector<string>("string[]", "string", engine);
	r=engine->RegisterGlobalFunction("void print(const int[] &in)", asFUNCTION(printv<i32>), asCALL_CDECL); assert(r>=0);
	r=engine->RegisterGlobalFunction("void print(const double[] &in)", asFUNCTION(printv<double>), asCALL_CDECL); assert(r>=0);
	r=engine->RegisterGlobalFunction("void print(const string[] &in)", asFUNCTION(printv<string>), asCALL_CDECL); assert(r>=0);

	r=engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print<string>), asCALL_CDECL); assert(r>=0);
	r=engine->RegisterGlobalFunction("void print(const int64 &in)", asFUNCTION(print<int64_t>), asCALL_CDECL); assert(r>=0);
	r=engine->RegisterGlobalFunction("void print(const uint64 &in)", asFUNCTION(print<uint64_t>), asCALL_CDECL); assert(r>=0);
	r=engine->RegisterGlobalFunction("void print(const double &in)", asFUNCTION(print<double>), asCALL_CDECL); assert(r>=0);
	r=engine->RegisterGlobalFunction("void print(const bool &in)", asFUNCTION(print<bool>), asCALL_CDECL); assert(r>=0);

	// extend string type by everything we need
	r=engine->RegisterObjectMethod("string", "void clear() const", asMETHOD(string,clear), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "bool empty() const", asMETHOD(string,empty), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "void erase(size_t, size_t)", asMETHODPR(string,erase, (size_t, size_t), string&), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find(const string &in, size_t)", asMETHODPR(string,find, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_first_of(string, size_t)", asMETHODPR(string,find_first_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_first_not_of(string, size_t)", asMETHODPR(string,find_first_not_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_last_of(string, size_t)", asMETHODPR(string,find_last_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_last_not_of(string, size_t)", asMETHODPR(string,find_last_not_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "string substr(size_t, size_t)", asMETHOD(string,substr), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "string &insert(size_t, const string &in)", asMETHODPR(string,insert,(size_t, const string&), string&), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "string &replace(size_t, size_t, const string &in, size_t, size_t)", asMETHODPR(string,replace,(size_t, size_t, const string &, size_t, size_t),string&), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "string &replace(size_t, size_t, const string &in)", asMETHODPR(string,replace,(size_t,size_t,const string&),string&), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterGlobalProperty("const size_t string__npos", (void*)&string::npos); assert(r>=0);

	// cctype
	r = engine->RegisterGlobalFunction("int isalnum(int)", asFUNCTIONPR(isalnum, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isalpha(int)", asFUNCTIONPR(isalpha, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int iscntrl(int)", asFUNCTIONPR(iscntrl, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isdigit(int)", asFUNCTIONPR(isdigit, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isgraph(int)", asFUNCTIONPR(isgraph, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int islower(int)", asFUNCTIONPR(islower, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isprint(int)", asFUNCTIONPR(isprint, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int ispunct(int)", asFUNCTIONPR(ispunct, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isupper(int)", asFUNCTIONPR(isupper, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isxdigit(int)", asFUNCTIONPR(isxdigit, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int tolower(int)", asFUNCTIONPR(tolower, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int toupper(int)", asFUNCTIONPR(toupper, (int), int), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("int isspace(int)", asFUNCTIONPR(isspace, (int), int), asCALL_CDECL); assert(r>=0);

	// cstdlib
	r = engine->RegisterGlobalFunction("double atof(string)", asFUNCTION(atof_wrap), asCALL_GENERIC); assert(r>=0);
	r = engine->RegisterGlobalFunction("int atoi(string)" , asFUNCTION(atol_wrap), asCALL_GENERIC); assert(r>=0);
	r = engine->RegisterGlobalFunction("int64 abs(int64)"   , asFUNCTION(labs), asCALL_CDECL); assert(r>=0);

	// to-string conversions
	r = engine->RegisterGlobalFunction("string itoa(int)"      , asFUNCTION(toa<int>), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("string dtoa(double)"   , asFUNCTION(toa<double>), asCALL_CDECL); assert(r>=0);
	// helper functions, logging, etc
	r = engine->RegisterEnum("log_level");
	r = engine->RegisterEnumValue("log_level", "LOG_ERR"    , LOG_ERR); assert(r>=0);
	r = engine->RegisterEnumValue("log_level", "LOG_WARNING", LOG_WARNING); assert(r>=0);
	r = engine->RegisterEnumValue("log_level", "LOG_INFO"   , LOG_INFO); assert(r>=0);
	r = engine->RegisterEnumValue("log_level", "LOG_DEBUG"  , LOG_DEBUG); assert(r>=0);
	r = engine->RegisterGlobalFunction("void log_string(const string &in, log_level)", asFUNCTION(log_string), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("const string& trim_string(string &in)", asFUNCTION(trim_string), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("bool validate_url(const string &in)", asFUNCTION(validate_url), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("const string& replace_all(string &in, const string &in, const string &in)", asFUNCTION(replace_all), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("string getenv(const string &in)", asFUNCTION(get_env_var), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("string filename_from_url(const string &in)", asFUNCTION(filename_from_url), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("const string& make_valid_filename(string &in)", asFUNCTION(make_valid_filename), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("string[] split_string(const string &in, const string &in, bool)", asFUNCTION(split_string), asCALL_CDECL); assert(r>=0);

	// register curl functionality
	r = engine->RegisterEnum("CURLoption"); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_URL", CURLOPT_URL); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_PORT", CURLOPT_PORT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_USERPWD", CURLOPT_USERPWD); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_RANGE", CURLOPT_RANGE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_WRITEFUNCTION", CURLOPT_WRITEFUNCTION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_POSTFIELDS", CURLOPT_POSTFIELDS); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_REFERER", CURLOPT_REFERER); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_FTPPORT", CURLOPT_FTPPORT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_USERAGENT", CURLOPT_USERAGENT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_LOW_SPEED_LIMIT", CURLOPT_LOW_SPEED_LIMIT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_LOW_SPEED_TIME", CURLOPT_LOW_SPEED_TIME); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_RESUME_FROM", CURLOPT_RESUME_FROM); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_COOKIE", CURLOPT_COOKIE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_HTTPHEADER", CURLOPT_HTTPHEADER); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_KEYPASSWD", CURLOPT_KEYPASSWD); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_WRITEHEADER", CURLOPT_WRITEHEADER); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_SSLVERSION", CURLOPT_SSLVERSION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_TIMEVALUE", CURLOPT_TIMEVALUE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_POST", CURLOPT_POST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_DIRLISTONLY", CURLOPT_DIRLISTONLY); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_TRANSFERTEXT", CURLOPT_TRANSFERTEXT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_PROGRESSFUNCTION", CURLOPT_PROGRESSFUNCTION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_AUTOREFERER", CURLOPT_AUTOREFERER); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_MAXREDIRS", CURLOPT_MAXREDIRS); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_FILETIME", CURLOPT_FILETIME); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_MAXCONNECTS", CURLOPT_MAXCONNECTS); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_CONNECTTIMEOUT", CURLOPT_CONNECTTIMEOUT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_HEADERFUNCTION", CURLOPT_HEADERFUNCTION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_HTTPGET", CURLOPT_HTTPGET); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_SSL_VERIFYHOST", CURLOPT_SSL_VERIFYHOST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_DNS_CACHE_TIMEOUT", CURLOPT_DNS_CACHE_TIMEOUT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_COOKIESESSION", CURLOPT_COOKIESESSION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_RESUME_FROM_LARGE", CURLOPT_RESUME_FROM_LARGE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_COOKIELIST", CURLOPT_COOKIELIST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_USERNAME", CURLOPT_USERNAME); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_PASSWORD", CURLOPT_PASSWORD); assert(r>=0);
	r = engine->RegisterEnumValue("CURLoption", "CURLOPT_NOPROXY", CURLOPT_NOPROXY); assert(r>=0);

	r = engine->RegisterEnum("CURLcode"); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_OK", CURLE_OK); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_UNSUPPORTED_PROTOCOL", CURLE_UNSUPPORTED_PROTOCOL); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FAILED_INIT", CURLE_FAILED_INIT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_URL_MALFORMAT", CURLE_URL_MALFORMAT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_COULDNT_RESOLVE_PROXY", CURLE_COULDNT_RESOLVE_PROXY); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_COULDNT_RESOLVE_HOST", CURLE_COULDNT_RESOLVE_HOST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_COULDNT_CONNECT", CURLE_COULDNT_CONNECT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_WEIRD_SERVER_REPLY", CURLE_FTP_WEIRD_SERVER_REPLY); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_REMOTE_ACCESS_DENIED", CURLE_REMOTE_ACCESS_DENIED); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_WEIRD_PASS_REPLY", CURLE_FTP_WEIRD_PASS_REPLY); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_WEIRD_PASV_REPLY", CURLE_FTP_WEIRD_PASV_REPLY); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_WEIRD_227_FORMAT", CURLE_FTP_WEIRD_227_FORMAT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_CANT_GET_HOST", CURLE_FTP_CANT_GET_HOST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_COULDNT_SET_TYPE", CURLE_FTP_COULDNT_SET_TYPE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_PARTIAL_FILE", CURLE_PARTIAL_FILE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_COULDNT_RETR_FILE", CURLE_FTP_COULDNT_RETR_FILE); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_HTTP_RETURNED_ERROR", CURLE_HTTP_RETURNED_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_WRITE_ERROR", CURLE_WRITE_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_OUT_OF_MEMORY", CURLE_OUT_OF_MEMORY); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_OPERATION_TIMEDOUT", CURLE_OPERATION_TIMEDOUT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_PORT_FAILED", CURLE_FTP_PORT_FAILED); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_COULDNT_USE_REST", CURLE_FTP_COULDNT_USE_REST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_RANGE_ERROR", CURLE_RANGE_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_HTTP_POST_ERROR", CURLE_HTTP_POST_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_SSL_CONNECT_ERROR", CURLE_SSL_CONNECT_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_BAD_DOWNLOAD_RESUME", CURLE_BAD_DOWNLOAD_RESUME); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FUNCTION_NOT_FOUND", CURLE_FUNCTION_NOT_FOUND); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_ABORTED_BY_CALLBACK", CURLE_ABORTED_BY_CALLBACK); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_BAD_FUNCTION_ARGUMENT", CURLE_BAD_FUNCTION_ARGUMENT); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_TOO_MANY_REDIRECTS", CURLE_TOO_MANY_REDIRECTS); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_PEER_FAILED_VERIFICATION", CURLE_PEER_FAILED_VERIFICATION); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_GOT_NOTHING", CURLE_GOT_NOTHING); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_RECV_ERROR", CURLE_RECV_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_SSL_CERTPROBLEM", CURLE_SSL_CERTPROBLEM); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_BAD_CONTENT_ENCODING", CURLE_BAD_CONTENT_ENCODING); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_LOGIN_DENIED", CURLE_LOGIN_DENIED); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_REMOTE_FILE_NOT_FOUND", CURLE_REMOTE_FILE_NOT_FOUND); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_FTP_BAD_FILE_LIST", CURLE_FTP_BAD_FILE_LIST); assert(r>=0);
	r = engine->RegisterEnumValue("CURLcode", "CURLE_CHUNK_FAILED", CURLE_CHUNK_FAILED); assert(r>=0);

	r = engine->RegisterEnum("curl_infotype"); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_TEXT", CURLINFO_TEXT); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_HEADER_IN", CURLINFO_HEADER_IN); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_HEADER_OUT", CURLINFO_HEADER_OUT); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_DATA_IN", CURLINFO_DATA_IN); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_DATA_OUT", CURLINFO_DATA_OUT); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_SSL_DATA_IN", CURLINFO_SSL_DATA_IN); assert(r>=0);
	r = engine->RegisterEnumValue("curl_infotype", "CURLINFO_SSL_DATA_OUT", CURLINFO_SSL_DATA_OUT); assert(r>=0);

	// curl-handles are just integers which represent the actual handle in the handle-map of ddapi
	r = engine->RegisterTypedef("CURL", "uint32");
	r = engine->RegisterGlobalFunction("CURL curl_init()", asFUNCTION(ddapi::new_handle), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("void curl_cleanup(CURL)", asFUNCTION(ddapi::delete_handle), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("CURLcode curl_setopt(CURL, CURLoption, ?&in)", asFUNCTION(ddapi::setopt_wrap), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("CURLcode curl_perform(CURL id)", asFUNCTION(ddapi::perform_wrap), asCALL_CDECL); assert(r>=0);


	// register DD functionality
	r = engine->RegisterEnum("download_status"); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_PENDING", DOWNLOAD_PENDING); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_INACTIVE", DOWNLOAD_INACTIVE); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_FINISHED", DOWNLOAD_FINISHED); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_RUNNING", DOWNLOAD_RUNNING); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_WAITING", DOWNLOAD_WAITING); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_DELETED", DOWNLOAD_DELETED); assert(r>=0);
	r = engine->RegisterEnumValue("download_status", "DOWNLOAD_RECONNECTING", DOWNLOAD_RECONNECTING); assert(r>=0);
	r = engine->RegisterEnum("plugin_status"); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_SUCCESS", PLUGIN_SUCCESS); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_ERROR", PLUGIN_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_LIMIT_REACHED", PLUGIN_LIMIT_REACHED); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_FILE_NOT_FOUND", PLUGIN_FILE_NOT_FOUND); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_CONNECTION_ERROR", PLUGIN_CONNECTION_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_SERVER_OVERLOADED", PLUGIN_SERVER_OVERLOADED); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_INVALID_HOST", PLUGIN_INVALID_HOST); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_CONNECTION_LOST", PLUGIN_CONNECTION_LOST); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_WRITE_FILE_ERROR", PLUGIN_WRITE_FILE_ERROR); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_AUTH_FAIL", PLUGIN_AUTH_FAIL); assert(r>=0);
	r = engine->RegisterEnumValue("plugin_status", "PLUGIN_CAPTCHA", PLUGIN_CAPTCHA); assert(r>=0);

	r = engine->RegisterObjectType("plugin_input", sizeof(plugin_input), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert(r>=0);
	r = engine->RegisterObjectType("plugin_output", sizeof(plugin_output), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_input", "string premium_password", offsetof(plugin_input,premium_password)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_input", "string premium_user", offsetof(plugin_input,premium_user)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_input", "string url", offsetof(plugin_input,url)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "string download_url", offsetof(plugin_output,download_url)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "string download_filename", offsetof(plugin_output,download_filename)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "bool allows_resumption", offsetof(plugin_output,allows_resumption)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "bool allows_multiple", offsetof(plugin_output,allows_multiple)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "bool offers_premium", offsetof(plugin_output,offers_premium)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "filesize_t file_size", offsetof(plugin_output,file_size)); assert(r>=0);
	r = engine->RegisterObjectProperty("plugin_output", "plugin_status file_online", offsetof(plugin_output,file_online)); assert(r>=0);
	r = engine->RegisterObjectBehaviour("plugin_input", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(plugin_input::cons), asCALL_CDECL_OBJLAST); assert(r>=0);
	r = engine->RegisterObjectBehaviour("plugin_input", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(plugin_input::dest), asCALL_CDECL_OBJLAST); assert(r>=0);
	r = engine->RegisterObjectBehaviour("plugin_output", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(plugin_output::cons), asCALL_CDECL_OBJLAST); assert(r>=0);
	r = engine->RegisterObjectBehaviour("plugin_output", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(plugin_output::dest), asCALL_CDECL_OBJLAST); assert(r>=0);
	// register the plugins interface(s)
}

ddapi::ddapi(const ddapi&) {

}

ddapi::~ddapi() {
	engine->Release();
}

void ddapi::start_engine() {
	if(ddapi::instance == 0)
		ddapi::instance = new ddapi;
	ddapi::reload_plugins();


}

void ddapi::shutdown() {
	if(ddapi::instance)
		delete ddapi::instance;
	ddapi::instance = 0;
}

static string exec_str;
void ddapi::exec(const string &command) {
	if(command != "!") {
		exec_str += command + "\n";
		return;
	}
	ddapi *d = ddapi::instance;
	d->engine->SetMessageCallback(asFUNCTION(as_msg_cb_cerr), 0, asCALL_CDECL);
	asIScriptModule *mod = d->engine->GetModule("DownloadDaemon console", asGM_CREATE_IF_NOT_EXISTS);
	ExecuteString(d->engine, exec_str.c_str(), mod);
	d->engine->SetMessageCallback(asFUNCTION(as_msg_cb_log), 0, asCALL_CDECL);
	exec_str.clear();
}
