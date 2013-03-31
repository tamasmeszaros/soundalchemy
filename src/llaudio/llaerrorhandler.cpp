/*
 * llaerrorhandler.cpp
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

#include "llaerrorhandler.h"

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>


using namespace llaudio;
using namespace std;

const char * llaErrorHandler::errors_[NERRORS] = { "",
		"Cannot open the specified audio stream",
		"Cannot write to the specified audio stream",
		"Cannot read from the specified audio stream",
		"Detection of devices was not entirely successful",
		"Detection of available PCM streams was not entirely successful",
		"Array index out of range",
		"The stream has to be opened before calling",
		"Incompatible buffer sizes", "No driver found",
		"Parameter can not be set or read",
		"Given parameter value differs from applied",
		"Cannot connect stream"
		"Unsupported by the audio engine",

};
#ifdef USE_EXCEPTIONS
llaErrorHandler::Exception::Exception(const char* e) {
	errstr_ = e;
}

const char* llaErrorHandler::Exception::what() throw() {
	return errstr_;
}
#endif

llaErrorHandler::llaErrorHandler() {
	last_error_ = E_OK;
	use_exceptions_ = false;
	enable_logging_ = false;
	exception_error_string_ = "";
	cleanup_func_ = NULL;
	detailed_logging_ = false;
	srcfile_ = "no source file";
	line_ = 0;
	enable_debug_ = false;
	fmterror_ = NULL;
}

llaErrorHandler::~llaErrorHandler() {
	if(fmterror_ != NULL) delete [] fmterror_;
}

void llaErrorHandler::setAdditionals(const char* srcfile, int line) {
	srcfile_ = srcfile;
	line_ = line;
}

const char*
llaErrorHandler::getStrError(TErrors err) {
	if (err < NERRORS)
		return errors_[err];

	return "";
}

void llaErrorHandler::error(TErrors err, const char* details, bool critical) {
	if(err == E_OK ) return;
	last_error_ = err;
	if (use_exceptions_) {
		exception_error_string_ = getFormatedError(ERROR, err,
				details);
#ifdef USE_EXCEPTIONS
		throw Exception(exception_error_string_);
#endif
	} else if(enable_logging_) {
		rawlog(ERROR, srcfile_, line_, err, details);
	}

	if(critical) this->terminate();
}


const char*
llaErrorHandler::getFormatedError(TErrorLevel level, TErrors err,
		const char* details) {

	if(fmterror_ != NULL) delete [] fmterror_;
	ostringstream ss;

	const char* strlevel[] = {
			"Info", "Debug", "Warning", "Error", "Fatal Error"
	};
	string generic = "";
	if(err != E_OK) {
		generic = string(getStrError(err)) + ": ";
	}

	if(detailed_logging_) {
		ss << strlevel[level] << ": " << srcfile_ <<  ": near line " << line_
				<< endl << generic << details << endl;
	} else
	ss << strlevel[level] << ": "<< generic << details << endl;

	int l = strlen(ss.str().c_str())+1;
	fmterror_ = new char[l];
	strcpy(fmterror_, ss.str().c_str());
	return fmterror_;
}

void llaErrorHandler::warning(TErrors err, const char* details) {
	if(enable_logging_) {
		rawlog(WARNING, srcfile_, line_, err, details);
	}
}

void llaErrorHandler::log(const char* description) {
	if(enable_logging_) {
		rawlog(INFO, "", 0, E_OK, description);
	}
}

void llaErrorHandler::debug(const char* description) {
	if(enable_logging_ && enable_debug_) {
		rawlog(DEBUGMSG, srcfile_, line_, E_OK, description);
	}
}

void llaErrorHandler::setCleanupHandler(cleanup_callback_t cleanup_func) {
	cleanup_func_ = cleanup_func;
}

void llaErrorHandler::terminate(void) {
	if(cleanup_func_ != NULL) cleanup_func_();
	exit(EXIT_FAILURE);
}

void llaErrorHandler::rawlog(TErrorLevel lvl, const char* srcfile, int line,
		TErrors err, const char* details) {
	if(lvl == INFO) {
		cout << "Info: " << details << endl;
	}

	else cerr << getFormatedError(lvl, err, details);
}


