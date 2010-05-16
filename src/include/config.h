#ifndef CONFIG_H
#define CONFIG_H

#define DD_CONF_DIR "/etc/downloaddaemon/"
#define DOWNLOADDAEMON
#define DOWNLOADDAEMON_VERSION "r753"

#define HAVE_UINT64_T
#define HAVE_SYSLOG_H
#define HAVE_INITGROUPS
#define HAVE_STAT64
#define USE_STD_THREAD

#define BACKTRACE_ON_CRASH

#ifdef __CYGWIN__
	#ifndef RTLD_LOCAL
		#define RTLD_LOCAL 0
	#endif
#endif

#ifdef HAVE_STAT64
        #define pstat stat64
#else
        #define pstat stat
#endif

#define DDCLIENT_GUI
#define DDCLIENT_GUI_VERSION "r753"


#endif // CONFIG_H
