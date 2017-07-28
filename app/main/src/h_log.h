#ifndef H_LOG_DOT_H
#define H_LOG_DOT_H

#include "h_types.h"

#ifndef LOG_PREFIX
#define LOG_PREFIX "Atom735 Log"
#endif

#include <android/log.h>
#define LOG_V(...) (__android_log_print(ANDROID_LOG_VERBOSE, LOG_PREFIX, __VA_ARGS__))
#define LOG_D(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_PREFIX, __VA_ARGS__))
#define LOG_I(...) (__android_log_print(ANDROID_LOG_INFO, LOG_PREFIX, __VA_ARGS__))
#define LOG_W(...) (__android_log_print(ANDROID_LOG_WARN, LOG_PREFIX, __VA_ARGS__))
#define LOG_E(...) (__android_log_print(ANDROID_LOG_ERROR, LOG_PREFIX, __VA_ARGS__))
#define LOG_F(...) (__android_log_print(ANDROID_LOG_FATAL, LOG_PREFIX, __VA_ARGS__))


#endif