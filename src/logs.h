/*
 * logs.h
 * 
 * Copyright (c) 2013 Mészáros Tamás.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 *     Mészáros Tamás - initial API and implementation
 */
#ifndef LOGGER_H_
#define LOGGER_H_

#ifdef ANDROID
#include <android/log.h>
#else
#include <cstdio>
#include <cstdlib>
#endif

namespace soundalchemy {

enum e_errorlevels
{
#ifdef ANDROID
	LEVEL_ERROR = ANDROID_LOG_ERROR,
	LEVEL_WARNING = ANDROID_LOG_WARN,
	LEVEL_INFO = ANDROID_LOG_INFO,
	LEVEL_DEBUG = ANDROID_LOG_DEBUG
#else
	LEVEL_ERROR,
	LEVEL_WARNING,
	LEVEL_INFO,
	LEVEL_DEBUG
#endif
};

void initLogs(void);
void freeLogs(void);
void enableDebug(void);

void log(enum e_errorlevels level, const char *fmt, ... );



#define CLIENTS_MAX 5

typedef enum e_errors
{
	E_OK,
	E_START,
	E_START_LISTENER,
	E_ALLOC,
	E_READ,
	E_QUEUE,
	E_DEMONIZE,
	E_AUDIO,
	E_PROCESS_INSTRUCTION,
	E_INDEX,
	NUMERR,
} TAlchemyError;

const char *const STR_ERRORS[NUMERR] = {
		"",
		"Cannot start the processing thread",
		"Cannot start the specified client listener",
		"Memory allocation failed",
		"Cannot read from the specified control input",
		"A message buffering error occurred",
		"Cannot start the program in the background (demonize)",
		"Cannot start the audio pipeline",
		"Cannot process instruction in time",
		"Index out of range or no such key exists",
};


}

#endif /* LOGGER_H_ */
