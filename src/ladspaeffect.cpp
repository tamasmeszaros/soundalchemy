/*
 * ladspaeffect.cpp
 *
 *  Created on: Apr 8, 2013
 *      Author: Mészáros Tamás
 */

#include "ladspaeffect.h"
#include <ladspa/utils.h>
#include <dlfcn.h>
#include <cstring>
#include <map>

namespace soundalchemy {

SoundEffect::TParamValue LADSPAEffect::LADSPAParam::getDefault(void) {
	LADSPA_Data v = 0.0f;

	if(parent_effect_ == NULL ) return v;
	else {
		getLADSPADefault(&range_hint_,
				(unsigned long) parent_effect_->getSampleRate(), &v);
	}

	return v;
}

LADSPAEffect::LADSPAEffect(const LADSPA_Descriptor& descriptor,
		llaudio::TSampleRate srate): SoundEffect(srate, descriptor.Label),
	plugin_descriptor_(descriptor) {

	plugin_handle_ = plugin_descriptor_.instantiate(&plugin_descriptor_,
			getSampleRate());

	// set up parameters and ports

	for(unsigned int i = 0; i < plugin_descriptor_.PortCount; i++) {
		const LADSPA_PortDescriptor& pd = plugin_descriptor_.PortDescriptors[i];
		const char * name = plugin_descriptor_.PortNames[i];
		const LADSPA_PortRangeHint& range_hint = plugin_descriptor_.PortRangeHints[i];
		if( LADSPA_IS_PORT_AUDIO(pd) ) {
			TPortDirection dir = LADSPA_IS_PORT_INPUT(pd)? INPUT_PORT : OUTPUT_PORT;
			LADSPAPort *port = new LADSPAPort(dir, name, range_hint,
					plugin_handle_, plugin_descriptor_, i);
			addPort(port);
		}
		else { // A control port. In the SoundEffect context this is a Param.
			LADSPAParam *p = new LADSPAParam( name, range_hint);

			plugin_descriptor_.connect_port(plugin_handle_, i, &(p->value_));
			addParam(p);
		}
	}
}

LADSPAEffect::~LADSPAEffect() {
	plugin_descriptor_.cleanup(plugin_handle_);
}


void LADSPAEffect::activate(void) {
	if( plugin_descriptor_.activate != NULL )
		plugin_descriptor_.activate(plugin_handle_);
}

void LADSPAEffect::deactivate(void) {
	if( plugin_descriptor_.deactivate != NULL )
		plugin_descriptor_.deactivate(plugin_handle_);
}

void LADSPAEffect::process(unsigned int sample_count) {
	plugin_descriptor_.run(plugin_handle_, sample_count);
}

LADSPAEffect* LADSPAEffect::loadPlugin(const char* library_file,
		const char* label, llaudio::TSampleRate sample_rate) {

	typedef std::map<std::string, void *>::iterator TCacheIt;
	static class HandleCache {
		std::map<std::string, void *> cache_;
	public:
		~HandleCache() {
			for( TCacheIt i = cache_.begin(); i != cache_.end(); i++ ) {
				dlclose(i->second);
			}
		}

		std::map<std::string, void *>* operator->(void) {
			return &cache_;
		}
	} handle_cache;


	LADSPAEffect *effect = NULL;
	//void* handle = loadLADSPAPluginLibrary(library_file);

	const LADSPA_Descriptor * descriptor;
	LADSPA_Descriptor_Function descriptorFunction;
	unsigned long pluginIndex = 0;

	dlerror();

	TCacheIt ci;
	if( (ci = handle_cache->find(label)) != handle_cache->end() ) {
		descriptorFunction = (LADSPA_Descriptor_Function) dlsym( ci->second,
			"ladspa_descriptor");
	}
	else {
		void* handle = loadLADSPAPluginLibrary(library_file);
		descriptorFunction
		= (LADSPA_Descriptor_Function)dlsym(handle,
						"ladspa_descriptor");

		if (!descriptorFunction) {
		const char * pcError = dlerror();
			if (pcError) {
			  soundalchemy::log(soundalchemy::LEVEL_ERROR,
				  "Unable to find ladspa_descriptor() function in plugin "
				  "library file \"%s\": %s.\n"
				  "Are you sure this is a LADSPA plugin file?\n",
				  library_file,
				  pcError);
			  return NULL;
			}
		}
		else {
			handle_cache->insert( std::pair<std::string, void*>(label, handle));
		}
	}


	do {
		descriptor = descriptorFunction(pluginIndex);
		if( descriptor != NULL && !strcmp(descriptor->Label, label)) {
			effect = new LADSPAEffect( *descriptor, sample_rate);
			break;
		}
		pluginIndex++;
	} while (descriptor != NULL );


	return effect;
}

} /* namespace soundalchemy */
