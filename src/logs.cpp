/*
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v2.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * 
 * Contributors:
 *     Mészáros Tamás - initial API and implementation
 */
#include "logs.h"
#include <pthread.h>
#include <cstdarg>
#include <string>


using namespace std;

namespace soundalchemy {

pthread_mutex_t logmutex;
bool display_logs = false;
bool debugs = false;



const char *MSGLEVELS[] = {
	"ERROR",
	"WARNING",
	"MESSAGE",
	"DEBUG"
};

void initLogs(void) {
	display_logs = true;
	pthread_mutex_init(&logmutex, NULL);
}

void enableDebug(void) {
	debugs = true;
}

void freeLogs(void) {
	pthread_mutex_destroy(&logmutex);
}

void log(enum e_errorlevels level, const char * fmt, ...) {
	if(!display_logs) return;
	if( !debugs && level == LEVEL_DEBUG ) return;

	va_list va;
	va_start(va, fmt);
	string ffmt;

	pthread_mutex_lock(&logmutex);
#ifdef ANDROID
	__android_log_vprint(level, "message_from_JNI", fmt, va);
#else
	ffmt = string(MSGLEVELS[level]) + ": " + fmt + "\n";
	vfprintf(stderr, ffmt.c_str(), va);
	fflush(stderr);
#endif
	pthread_mutex_unlock(&logmutex);
}
}
