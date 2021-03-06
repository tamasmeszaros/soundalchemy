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
#include "dspserver.h"
#include "androidconnector.h"
#include <iostream>

using namespace soundalchemy;
using namespace std;

int main(int argc, const char * argv[] )
{
#ifdef DEBUG_LLAUDIO
	llaErrorHandler eh;
	eh.enableLogging(true, true);
	eh.useExceptions(true);

	try {
		llaDeviceManager& llalib = llaDeviceManager::getInstance(eh);
		for(llaDeviceIterator i = llalib.getDeviceIterator(); !i.end(); i++) {
			cout << i->getName() << ": " << i->getName(true) << endl;
			for(llaDevice::IStreamIterator j = i->getInputStreamIterator(); !j.end(); j++) {
				cout <<"\t"<< j->getName() << " Capture" << endl;

			}

			for(llaDevice::OStreamIterator j = i->getOutputStreamIterator(); !j.end(); j++) {
				cout <<"\t"<< j->getName() << " Playback" <<endl;

			}
		}

		//llaFileStream *input = llalib->getFileStream("/home/quarky/Downloads/runaway.wav");
		llaInputStream& input = llalib.getDevice("RP250").getInputStream();
		input.setChannelCount(CH_STEREO);

		llaOutputStream& output = llalib.getDevice("RP250").getOutputStream();

		llaAudioPipe audio_pipe;

		audio_pipe.connectStreams(input, output);

		llalib.destroy();
	} catch (llaErrorHandler::Exception& err) {
		cerr << err.what();
	}
#else

	initLogs();
	//enableDebug();
	log(LEVEL_INFO, "Sound Alchemy started in service mode");
	log(LEVEL_INFO, "buffer size: %d", llaudio::DEFAULT_BUFFER_SIZE);
	DspServer dspserver;

	AndroidConnector android;
	dspserver.listenOn(android);

	dspserver.startListening();

	freeLogs();
#endif

	return 0;
}
