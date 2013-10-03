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
#ifndef LADSPAEFFECT_H_
#define LADSPAEFFECT_H_

#include "soundeffect.h"
#ifdef ANDROID
#include <ladspa/ladspa.h>
#else
#include <ladspa.h>
#endif
#include <list>

namespace soundalchemy {

class LADSPAEffect: public soundalchemy::SoundEffect {

	class LADSPAParam: public Param {
		const LADSPA_PortRangeHint& range_hint_;

	public:
		LADSPAParam(std::string name, const LADSPA_PortRangeHint& range_hint):
			Param(name),
			range_hint_(range_hint) {}

		virtual TParamValue getValue() { return (TParamValue) value_; }

		virtual void setValue(TParamValue value) {
			value_ = (LADSPA_Data) value;
		}

		virtual TParamValue getDefault(void);

		virtual TParamValue getMin(void) { return range_hint_.LowerBound; }
		virtual TParamValue getMax(void) { return range_hint_.UpperBound; }
		virtual bool isLogarithmic() {
			return LADSPA_IS_HINT_LOGARITHMIC(range_hint_.HintDescriptor);
		}

		virtual TParamType getType() {
			TParamType type = PARAM_CONTINOUS;

			if( LADSPA_IS_HINT_INTEGER(range_hint_.HintDescriptor) )
				type = PARAM_INTEGER;
			else if( LADSPA_IS_HINT_TOGGLED(range_hint_.HintDescriptor) )
				type = PARAM_TOGGLE;
			else if( LADSPA_IS_HINT_SAMPLE_RATE(range_hint_.HintDescriptor))
				type = PARAM_SAMPLE_RATE;

			return type;
		}

		LADSPA_Data value_;
	};

	class LADSPAPort: public Port {
		const LADSPA_PortRangeHint& range_hint_;
		LADSPA_Data *buffer_;
		LADSPA_Handle plugin_handle_;
		const LADSPA_Descriptor& plugin_descriptor_;
		unsigned int port_index_;

	public:
		LADSPAPort(TPortDirection dir, std::string name,
				const LADSPA_PortRangeHint& range_hint,
				LADSPA_Handle plugin_handle,
				const LADSPA_Descriptor& plugin_descriptor_, unsigned int index):
			Port(dir, name),
			range_hint_(range_hint_),
			plugin_handle_(plugin_handle),
			plugin_descriptor_(plugin_descriptor_),
			port_index_(index) {}

		virtual void connect(Port& port) {
			buffer_ = (LADSPA_Data*) port.getBuffer();
			plugin_descriptor_.connect_port(plugin_handle_, port_index_, buffer_);
		}

		virtual TSample* getBuffer() { return (TSample*) buffer_; }

	};

	LADSPA_Handle plugin_handle_;
	const LADSPA_Descriptor& plugin_descriptor_;

public:


	LADSPAEffect(const LADSPA_Descriptor& descriptor, llaudio::TSampleRate srate);
	virtual ~LADSPAEffect();

	virtual void process(unsigned int sample_count);

	virtual void activate(void);
	virtual void deactivate(void);

	static LADSPAEffect* loadPlugin(const char* library_file,
			const char* label, llaudio::TSampleRate sample_rate);


};

} /* namespace soundalchemy */
#endif /* LADSPAEFFECT_H_ */
