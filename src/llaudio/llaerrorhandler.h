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
#ifndef LLAERRORHANDLER_H_
#define LLAERRORHANDLER_H_

#include "predef.h"

namespace llaudio {

/**
 * @details This class handles logging errors and other messages from the
 * library. The log output can be customized by subclassing and overloading the
 * rawlog method. If exceptions are enabled, it supports throwing exceptions
 * on reporting an error.
 */
class llaErrorHandler {
public:

#ifdef USE_EXCEPTIONS
	/**
	 * A class for throwing as an exception
	 */
	class Exception: public std::exception {
	public:
		Exception(const char* e);

		virtual const char* what() throw();
	private:
		const char* errstr_;
	};
#endif

	/// if a critical error is reported by the library it exits but calls
	/// this method first to do some cleanup stuff.
	typedef void (*cleanup_callback_t) (void);

	/**
	 * Levels of log message
	 */
	typedef enum e_level {
		INFO,         //!< A standard information message
		DEBUGMSG,     //!< A debug message
		WARNING,      //!< A warning message
		ERROR,        //!< An error message
		CRITICAL_ERROR//!< A critical error
	} TErrorLevel;

	/// default constructor
	llaErrorHandler();

	/// default destructor
	virtual ~llaErrorHandler();

	/**
	 *
	 * @param err
	 * @param details
	 * @param critical
	 */
	void error(TErrors err, const char *details, bool critical = false);

	/**
	 *
	 * @param err
	 * @param details
	 */
	void warning(TErrors err, const char *details);

	/**
	 *
	 * @param description
	 */
	void log(const char* description);

	/**
	 *
	 * @param description
	 */
	void debug(const char* description);

	/**
	 *
	 * @param srcfile
	 * @param line
	 */
	void setAdditionals(const char* srcfile, int line);

	/**
	 *
	 * @param toggle
	 * @param detailed
	 */
	void enableLogging(bool toggle = true, bool detailed = false) {
		enable_logging_ = toggle;
		detailed_logging_ = detailed;
	}

	/**
	 *
	 * @param toggle
	 */
	void enableDebugMessages(bool toggle = true) { enable_debug_ = toggle; }

	/**
	 *
	 * @param toggle
	 */
	void useExceptions(bool toggle) { use_exceptions_ = toggle; }

	/**
	 *
	 * @return
	 */
	TErrors getLastError(void) { return last_error_; }

	/**
	 *
	 * @param err
	 * @return
	 */
	const char* getStrError(TErrors err);

	/**
	 *
	 * @param level
	 * @param err
	 * @param details
	 * @return
	 */
	const char* getFormatedError(TErrorLevel level, TErrors err,
			const char* details);

	/**
	 *
	 * @param cleanup_func
	 */
	void setCleanupHandler(cleanup_callback_t cleanup_func);

	/**
	 *
	 */
	void terminate(void);

protected:

	/**
	 *
	 * @param lvl
	 * @param srcfile
	 * @param line
	 * @param err
	 * @param details
	 */
	virtual void rawlog(TErrorLevel lvl, const char* srcfile, int line,
			TErrors err, const char* details);


	TErrors last_error_;
	bool use_exceptions_;
	bool enable_logging_;
	bool detailed_logging_;
	bool enable_debug_;
	const char* exception_error_string_;
	const char* srcfile_;
	int line_;

	char* fmterror_;

private:
	cleanup_callback_t cleanup_func_;
	static const char * errors_[NERRORS];
};

}


#endif /* LLAERRORHANDLER_H_ */
