#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine USE_STD_THREAD
#cmakedefine DD_CONF_DIR @DD_CONF_DIR_OPT@
#cmakedefine HAVE_UINT64_T
#cmakedefine HAVE_SYSLOG_H
#cmakedefine DOWNLOADDAEMON
#cmakedefine DOWNLOADDAEMON_VERSION "@VERSION@"

#endif // CONFIG_H
