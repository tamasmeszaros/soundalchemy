/*
 * llaudio_predef.h
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
#ifndef LLAUDIO_PREDEF_H_
#define LLAUDIO_PREDEF_H_
#include <cstdlib>
#include <stdint.h>

/// The definitions will contain exception handling
#define USE_EXCEPTIONS


#ifdef USE_EXCEPTIONS
// defining an exception throwing macro constant
#include <exception>
#define __LLATHROW throw(llaErrorHandler::Exception)
#else
#define __LLATHROW
#endif


/// the namespace for the whole library -- llaudio
namespace llaudio {

////////////////////////////////////////////////////////////////////////////////
/// classes (interfaces) and functions which are used in the library:
////////////////////////////////////////////////////////////////////////////////

class llaDriver;
class llaDevice;
class llaErrorHandler;
class llaStream;
class llaInputStream;
class llaOutputStream;
class llaFileStream;
class llaAudioPipe;
class llaDeviceManager;

class llaNullDevice;
class llaNullStream;

/**
 * Function to determine the architecture endianness
 * @return Returns true if the Architecture is big endian
 */
bool isBigEndianArch(void);

////////////////////////////////////////////////////////////////////////////////
/// Types
////////////////////////////////////////////////////////////////////////////////

/// A general unsigned type used throughout the code
typedef unsigned int TSize;

/// Holds sampling rate
typedef unsigned long TSampleRate;

/// Holds device ID-s
typedef const char * TDeviceId;

/// Holds Stream ID-s
typedef int TStreamId;

/**
 * Named constants for error states and warnings
 */
typedef enum e_errors {
	E_OK,                     //!< Everything is OK
	E_OPEN_STREAM,            //!< Error on opening a stream
	E_WRITE_STREAM,           //!< Writing streams
	E_READ_STREAM,            //!< Reading from a stream
	E_DETECT_DEVICES,         //!< Error on detecting devices
	E_DETECT_STREAMS,         //!< Error on detecting streams
	E_INDEX_RANGE,            //!< An out of index range error
	E_DEVICE_CLOSED,          //!< Manipulating a closed or uninitialized resource
	E_BUFFER_DISMATCH,        //!< Incompatibility of buffers (deprecated)
	E_DRIVER,                 //!< Error with the audio engine (driver)
	E_STREAM_CONFIG,          //!< Error when configuring a stream
	E_STREAM_PARAM_DIFFERENCE,//!< A parameter value differs from the requested
	E_STREAM_INCOMPATIBLE,    //!< Incompatible streams are connected
	E_UNIMPLEMENTED,          //!< An unimplemented method called
	//...
	NERRORS                   //!< NERRORS
} TErrors;

/**
 * Named constants for generally used channel numbers
 */
typedef enum e_channels  {
	CH_NONE = 0,  //!< No channels
	CH_MONO = 1,  //!< MONO
	CH_STEREO = 2,//!< STEREO
	CH_40 = 4,    //!< 4.0
	CH_41 = 5,    //!< 4.1
	CH_51 = 6,    //!< 5.1
	CH_71 = 8,    //!< 7.1
	CH_91 = 10,   //!< 9.1
	// ...
	CH_MAX = CH_91//!< Maximal channels number
} TChannels;

/**
 * Drivers or engines that can be used by the library for audio device detection
 * and manipulation
 */
typedef enum e_drivers {
	DRIVER_SALSA, DRIVER_TINYALSA, DRIVER_NATIVE, DRIVER_NONE
} TDrivers;

////////////////////////////////////////////////////////////////////////////////
/// Constants
////////////////////////////////////////////////////////////////////////////////

const TSampleRate SR_8000 = 8000;
const TSampleRate SR_11025 = 11025;
const TSampleRate SR_CD_QUALITY_44100 = 44100;
const TSampleRate SR_ADVANCED_48000 = 48000;
const TSampleRate SR_PROFESSIONAL_96000 = 96000;

/* Default values */
const TSampleRate SR_DEFAULT = SR_CD_QUALITY_44100;
const TStreamId STREAM_ID_DEFAULT = 0;
const TDrivers DEFAULT_DRIVER = DRIVER_SALSA;
const TSize DEFAULT_BUFFER_SIZE = 512;
const TChannels CH_DEFAULT = CH_MONO;

extern llaNullDevice LLA_NULL_DEVICE;
extern llaNullStream LLA_NULL_STREAM;

/**
 * A macro which has to be used for writing messages (errors, warning, debug)
 * to the user. The final output can be customized with the llaErrorHandler
 */
#define  LOGGER() { \
	llaErrorHandler& eh = llaDeviceManager::getErrorHandler(); \
	 eh.setAdditionals(__FILE__, __LINE__); \
	} llaDeviceManager::getErrorHandler()


}

#endif /* LLAUDIO_PREDEF_H_ */
