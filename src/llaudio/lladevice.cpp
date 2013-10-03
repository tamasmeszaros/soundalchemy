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
#include "llaudio.h"
#include "defaultcontainer.h"

namespace llaudio {

llaNullDevice LLA_NULL_DEVICE;
llaNullStream LLA_NULL_STREAM;

llaDevice::llaDevice() {
	input_stream_list_ = new DefaultIStreamContainer();
	output_stream_list_ = new DefaultOStreamContainer();
}

llaDevice::llaDevice(IStreamList *istreams, OStreamList *ostreams) {
	input_stream_list_ = istreams;
	output_stream_list_ = ostreams;
}

llaDevice::~llaDevice() {
//	for(IStreamIterator it = getInputStreamIterator(); !it.end(); it++ )
//		delete *it;
	delete input_stream_list_;

//	for(OStreamIterator it = getOutputStreamIterator(); !it.end(); it++ )
//			delete *it;
	delete output_stream_list_;

}

llaInputStream& llaDevice::getInputStream(TStreamId id,
		TSampleRate sample_rate, TChannels channels, TSize buffer_size) {

	llaInputStream * is = input_stream_list_->find(id);
	if (is == NULL) {
		LOGGER().error(E_INDEX_RANGE, "No stream is found with the given id!");
		return LLA_NULL_STREAM;
	}

	is->setSampleRate(sample_rate);
	is->setChannelCount(channels);
	//is->setBufferLength(buffer_size);

	return *is;
}

llaOutputStream& llaDevice::getOutputStream(TStreamId id,
		TSampleRate sample_rate, TChannels channels, TSize buffer_size) {

	llaOutputStream *os = output_stream_list_->find(id);
	if (os == NULL) {
		LOGGER().error(E_INDEX_RANGE, "No stream is found with the given id!");
		return LLA_NULL_STREAM;
	}

	os->setSampleRate(sample_rate);
	os->setChannelCount(channels);
	//os->setBufferLength(buffer_size);

	return *os;
}

llaDevice::IStreamIterator llaDevice::getInputStreamIterator(void) {

	return input_stream_list_->getIterator();
}

llaDevice::OStreamIterator llaDevice::getOutputStreamIterator(void) {

	return output_stream_list_->getIterator();
}

}
