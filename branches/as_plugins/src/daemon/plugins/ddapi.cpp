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


#include "../tools/helperfunctions.h"

#include <sstream>
#include <cassert>
#include <iostream>
#include <cstring>
#include <vector>
using namespace std;

#ifdef AS_MAX_PORTABILITY
#error DownloadDaemon is not able to detect your hardware correctly and will therefore not run. Feel free to make a post in the DownloadDaemon forums and explain the details \
of your hardware. A port is probably pretty simple.
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

template <typename T>
void print(const T &val) {
	cout << val;
}

template <typename T>
void typedef_inttype(T dummy, asIScriptEngine *e, const string &type_name, bool _signed = true) {
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

asDECLARE_FUNCTION_WRAPPER(atof_wrap, atof);
asDECLARE_FUNCTION_WRAPPER(atol_wrap, atol);

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
	RegisterVector<int>("int[]", "int", engine);
	RegisterVector<double>("double[]", "double", engine);
	RegisterVector<string>("string[]", "string", engine);
	typedef_inttype((size_t)0, engine, "size_t", false);
	typedef_inttype((time_t)0, engine, "time_t", false);

	r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print<string>), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("void print(const int64 &in)", asFUNCTION(print<int64_t>), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("void print(const uint64 &in)", asFUNCTION(print<uint64_t>), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("void print(const double &in)", asFUNCTION(print<double>), asCALL_CDECL); assert(r>=0);
	r = engine->RegisterGlobalFunction("void print(const bool &in)", asFUNCTION(print<bool>), asCALL_CDECL); assert(r>=0);

	// extend string type by everything we need
	r=engine->RegisterObjectMethod("string", "void clear() const", asMETHOD(string,clear), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "bool empty() const", asMETHOD(string,empty), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "void erase(size_t, size_t)", asMETHODPR(string,erase, (size_t, size_t), string&), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find(string, size_t)", asMETHODPR(string,find, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_first_of(string, size_t)", asMETHODPR(string,find_first_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_first_not_of(string, size_t)", asMETHODPR(string,find_first_not_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_last_of(string, size_t)", asMETHODPR(string,find_last_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "size_t find_last_not_of(string, size_t)", asMETHODPR(string,find_last_not_of, (const string&, size_t) const, size_t), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterObjectMethod("string", "string substr(size_t n, size_t n)", asMETHOD(string,substr), asCALL_THISCALL); assert(r>=0);
	r=engine->RegisterGlobalProperty("const size_t string__npos", (void*)&string::npos); assert(r>=0);
	// TODO: more string methods, maybe vector

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

	// register curl functionality

	// register DD functionality


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
	r = engine->RegisterGlobalFunction("string[] split_string(const string &in, const string &in, bool)", asFUNCTION(split_string), asCALL_CDECL); assert(r>=0); // TODO: no vectors yet
}

ddapi::ddapi(const ddapi&) {

}

void ddapi::start_engine() {
	if(ddapi::instance == 0)
		ddapi::instance = new ddapi;


}

void ddapi::shutdown() {
	if(ddapi::instance)
		delete ddapi::instance;
	ddapi::instance = 0;
}

void ddapi::exec(const string &command) {
	ddapi *d = ddapi::instance;
	d->engine->SetMessageCallback(asFUNCTION(as_msg_cb_cerr), 0, asCALL_CDECL);
	asIScriptModule *mod = d->engine->GetModule("DownloadDaemon console", asGM_CREATE_IF_NOT_EXISTS);
	ExecuteString(d->engine, command.c_str(), mod);
	d->engine->SetMessageCallback(asFUNCTION(as_msg_cb_log), 0, asCALL_CDECL);
}



// implementation
#if 0
// the code below are just my first experiments with angelscript. It's only partially
// functional and does absolutely not what you want. Just ignore it, it's there for my
// inspiration
#include <assert.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <ctime>
#include <sstream>

using namespace std;

#include "aswrappedcall.h"
#include "scriptclib.h"
#include "scriptbuilder.h"


asDECLARE_FUNCTION_WRAPPER(remove_wrap, remove);
asDECLARE_FUNCTION_WRAPPER(rename_wrap, rename);

asDECLARE_FUNCTION_WRAPPER(system_wrap, system);

string getenv_wrap(const string &v) { const char *c = getenv(v.c_str()); return (c ? string(c) : ""); }
time_t time_wrap() { return time(0); }
string asctime_wrap(struct tm timeptr) { char *c = asctime(&timeptr); return (c ? string(c) : ""); }
//string ctime_wrap(time_t timer) { char *c = ctime(&timer); return (c ? string(c) : ""); }
//struct tm gmtime_wrap(time_t timer) { struct tm *t = gmtime(&timer); if(t) return *t; struct tm e; memset(&e, 0, sizeof(e)); return e; }



void RegisterScriptClib(asIScriptEngine *engine)
{
	int r;


	r = engine->RegisterGlobalFunction("int system(string)" , asFUNCTION(system_wrap), asCALL_GENERIC); assert( r >= 0 );
	
	r = engine->RegisterGlobalFunction("int rand()"                     , asFUNCTION(rand), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void srand(uint)"               , asFUNCTION(srand), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("string getenv(const string &in)", asFUNCTION(getenv_wrap), asCALL_CDECL); assert( r >= 0 );

	
	// ctime (only time_t stuff implemented, no struct tm
	r = engine->RegisterObjectType("time_t"   , sizeof(time_t)   , asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectType("size_t"   , sizeof(size_t)   , asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	//r = engine->RegisterObjectType("struct tm", sizeof(struct tm), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);     assert( r >= 0 );
	
	//r = engine->RegisterGlobalFunction("double difftime(time_t, time_t);" , asFUNCTION(difftime), asCALL_CDECL); assert( r >= 0 );
	//r = engine->RegisterGlobalFunction("time_t time()"                    , asFUNCTION(time), asCALL_CDECL); assert( r >= 0 );
	//r = engine->RegisterGlobalFunction("string asctime(struct tm)"      , asFUNCTION(asctime_wrap), asCALL_CDECL); assert( r >= 0 );
	//r = engine->RegisterGlobalFunction("string ctime(time_t)"           , asFUNCTION(ctime_wrap), asCALL_CDECL); assert( r >= 0 );
	//r = engine->RegisterGlobalFunction("struct tm gmtime(time_t)"       , asFUNCTION(gmtime_wrap), asCALL_CDECL); assert( r >= 0 );
	
	

}


#include "/home/ben/coding/downloaddaemon/trunk/src/include/ddcurl.h"

ddcurl* ddcurl_factory() {
	return new ddcurl(true);
}

template <typename T>
CURLcode c_setopt(ddcurl *_this, CURLoption opt, T val) {
	//return _this->setopt(opt, val);
	return CURLE_OK;
}
/*
CURLcode c_setopt(ddcurl *_this, CURLoption opt, const string &val) {
	if(opt == CURLOPT_WRITEFUNCTION)
		_this->as_writecb = val;
	else if(opt == CURLOPT_PROGRESSFUNCTION)
		_this->as_progresscb = val;
	else
		return _this->setopt(opt, val);
	return CURLE_OK;
}*/

void RegisterScriptCurl(asIScriptEngine *engine) {
	int r;
	//r = engine->RegisterObjectType("CURL", sizeof(CURL*), asOBJ_VALUE | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	//CScriptBuilder builder;
	//builder.StartNewModule(engine, "ddcurl");
	//builder.AddSectionFromFile("curl.as");
	//builder.BuildModule();

	r = engine->RegisterObjectType("ddcurl", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ddcurl", asBEHAVE_FACTORY, "ddcurl@ f()", asFUNCTION(ddcurl_factory), asCALL_CDECL); assert (r >= 0);
	r = engine->RegisterObjectBehaviour("ddcurl", asBEHAVE_ADDREF, "void f()", asMETHOD(ddcurl,AddRef), asCALL_THISCALL); assert (r >= 0);
	r = engine->RegisterObjectBehaviour("ddcurl", asBEHAVE_RELEASE, "void f()", asMETHOD(ddcurl,Release), asCALL_THISCALL); assert (r >= 0);
	r = engine->RegisterObjectMethod("ddcurl", "void init()", asMETHOD(ddcurl, init), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ddcurl", "void cleanup()", asMETHOD(ddcurl, cleanup), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ddcurl", "void perform()", asMETHOD(ddcurl, perform), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ddcurl", "int setopt(int, string)", asFUNCTION(c_setopt<string>), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ddcurl", "int setopt(int, int)", asFUNCTION(c_setopt<long>), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ddcurl", "int setopt(int, uint64)", asFUNCTION(c_setopt<curl_off_t>), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ddcurl", "int setopt(int, const ?&in)", asFUNCTION(c_setopt<void*>), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	
}


END_AS_NAMESPACE

#endif
