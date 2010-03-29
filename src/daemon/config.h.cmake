#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine USE_STD_THREAD
#cmakedefine DD_CONF_DIR @DD_CONF_DIR_OPT@
#cmakedefine DOWNLOADDAEMON
#cmakedefine DOWNLOADDAEMON_VERSION "@VERSION@"

#cmakedefine HAVE_UINT64_T
#cmakedefine HAVE_SYSLOG_H
#cmakedefine HAVE_INITGROUPS
#cmakedefine BACKTRACE_ON_CRASH

#ifdef __CYGWIN__
	#ifndef RTLD_LOCAL
		#define RTLD_LOCAL 0
	#endif
#endif



#endif // CONFIG_H
