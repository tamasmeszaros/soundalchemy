#include <string.h>
/*
 * jni_main.cpp
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
#include <stdlib.h>
#include <stdio.h>

#include <jni.h>

#include "logs.h"
#include "llaudio.h"


using namespace llaudio;
using namespace soundalchemy;

#if defined(__cplusplus)
extern "C" {
#endif



/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/hellojni/HelloJni.java
 */
void Java_cz_vutbr_fit_sound_alchemy_MainActivity_PlayFile( JNIEnv* env,
                                                  jobject thiz )
{
	initLogs();
	log(LEVEL_INFO, "Itt vagyok");

	class LLAErrorHandler: public llaErrorHandler {
	public:
		virtual void rawlog(TErrorLevel lvl, const char* srcfile, int line,
				TErrors err, const char* details){
			e_errorlevels saerr;
			switch(lvl) {
			case ERROR:
			case CRITICAL_ERROR: saerr = LEVEL_ERROR; break;
			case DEBUGMSG: saerr = LEVEL_DEBUG; break;
			case INFO: saerr = LEVEL_INFO; break;
			case WARNING: saerr = LEVEL_WARNING; break;
			}
			soundalchemy::log(saerr, getFormatedError(lvl, err, details));
		}
	} llaEH;
	llaDeviceManager* lla = llaDeviceManager::getInstance(&llaEH);

	llaEH.enableLogging(true, true);
	llaEH.enableDebugMessages(true);
	llaDevice *dev = lla->getDevice("RP250");
	if(dev == NULL) {
		log(LEVEL_WARNING, "Nemjooo");
		delete lla;
		return;
	}

	for(llaDevice::OStreamIterator it = dev->getOutputStreamIterator();
			!it.end(); it++) {
		char str[6];
		sprintf(str, "%d", it->getId());
		log(LEVEL_INFO, str);
	}


	llaOutputStream *out = dev->getOutputStream();
	if(out == NULL) {
		log(LEVEL_WARNING, "Nemjooo");
		delete lla;
		return;
	}

	out->open();
//	out->close();
//	llaFileStream *runaway = lla->getFileStream("/sdcard/music/runaway.wav");
//	llaAudioBuffer buffer(1024);
//	llaOutputStream *output = dev->getOutputStream();
//	buffer.connectStreams(runaway, output);
	delete lla;
	freeLogs();
}

void Java_cz_vutbr_fit_sound_alchemy_MainActivity_stopSoundServer( JNIEnv* env,
                                                  jobject thiz )
{


}



#if defined(__cplusplus)
}  /* extern "C" */
#endif
