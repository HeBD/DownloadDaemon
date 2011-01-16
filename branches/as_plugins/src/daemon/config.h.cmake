#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine DD_CONF_DIR @DD_CONF_DIR_OPT@
#cmakedefine DOWNLOADDAEMON
#cmakedefine DOWNLOADDAEMON_VERSION "@VERSION@"

#cmakedefine HAVE_UINT64_T
#cmakedefine HAVE_SYSLOG_H
#cmakedefine HAVE_INITGROUPS
#cmakedefine HAVE_STAT64
#cmakedefine USE_STD_THREAD

#cmakedefine BACKTRACE_ON_CRASH

#ifdef __CYGWIN__
	#ifndef RTLD_LOCAL
		#define RTLD_LOCAL 0
	#endif
#endif

#if defined(HAVE_STAT64) && !defined(__APPLE__)
        #define pstat stat64
#else
        #define pstat stat
#endif

// We need datatypes with a clearly defined length...
#include <climits>

// 64bit integers
#ifdef HAVE_UINT64_T
	#include <stdint.h>
	typedef uint64_t ui64;
	typedef int64_t i64;
#elif defined(LLONG_MAX) && LLONG_MAX == 9223372036854775807
	typedef unsigned long long ui64;
	typedef long long i64;
#elif defined(LONG_MAX) && LONG_MAX == 9223372036854775807 /* 64bit long */
	typedef unsigned long ui64;
	typedef long u64;
#else
	#warning DownloadDaemon was not able to find a 64bit integer type on your platform. This is very bad and may
	#warning cause trouble running DownloadDaemon. Please report this -- maybe its just a problem with the checks.
	typedef unsigned long ui64;
	typedef long u64;
	#define NO_INT64
#endif

// 32bit integers
#if defined(INT_MAX) && INT_MAX == 2147483647
	typedef unsigned int ui32;
	typedef int i32;
#elif defined(LONG_MAX) && LONG_MAX == 2147483647
	typedef unsigned long ui32;
	typedef long i32;
#endif

#ifdef NO_INT64
	typedef double filesize_t;
#else
	typedef ui64 filesize_t;
#endif


#endif // CONFIG_H
