/*
 * main.cpp
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


#include "dspserver.h"
#include "androidconnector.h"
#include <iostream>

using namespace soundalchemy;
using namespace std;

int main(int argc, const char * argv[] )
{
//	llaErrorHandler eh;
//	eh.enableLogging(true, true);
//	eh.useExceptions(true);
//
//	try {
//		llaDeviceManager* llalib = llaDeviceManager::getInstance(&eh);
//		for(llaDeviceIterator i = llalib->getDeviceIterator(); !i.end(); i++) {
//			cout << i->getName() << ": " << i->getName(true) << endl;
//			for(llaDevice::IStreamIterator j = i->getInputStreamIterator(); !j.end(); j++) {
//				cout <<"\t"<< j->getName() << " Capture" << endl;
//
//			}
//
//			for(llaDevice::OStreamIterator j = i->getOutputStreamIterator(); !j.end(); j++) {
//				cout <<"\t"<< j->getName() << " Playback" <<endl;
//
//			}
//		}
//
//		//llaFileStream *input = llalib->getFileStream("/home/quarky/Downloads/runaway.wav");
//		llaInputStream *input = llalib->getDevice("Intel")->getInputStream();
//		input->setChannelCount(CH_STEREO);
//
//		llaOutputStream * output = llalib->getDevice("Intel")->getOutputStream();
//
//		llaAudioBuffer buffer(32);
//
//		buffer.connectStreams(input, output);
//
//		delete llalib;
//	} catch (llaErrorHandler::Exception& err) {
//		cerr << err.what();
//	}

	initLogs();
	//enableDebug();
	log(LEVEL_INFO, "Sound Alchemy started in service mode");
	DspServer dspserver;

	AndroidConnector android;
	dspserver.listenOn(android);

	dspserver.startListening();

	freeLogs();
}
