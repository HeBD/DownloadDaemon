#ifndef DDAPI_H
#define DDAPI_H

#include "angelscript.h"
#include <string>

class ddapi {
public:
	static void start_engine();
	static void shutdown();
	static void exec(const std::string& command);

private:
	ddapi();
	ddapi(const ddapi &);
	static ddapi *instance;

	asIScriptEngine *engine;


};

// RegisterDDApi...(asIScriptEngine *engine), maybe a class to access plugin-details, etc


#endif
