/*
 * ladspaeffect.h
 *
 *  Created on: Apr 8, 2013
 *      Author: Mészáros Tamás
 */

#ifndef LADSPAEFFECT_H_
#define LADSPAEFFECT_H_

#include "soundeffect.h"
#include <list>

namespace soundalchemy {

class LADSPAEffect: public soundalchemy::SoundEffect {
public:
	typedef std::list<LADSPAEffect*> TLADSPAPluginLibrary;
	typedef TLADSPAPluginLibrary::iterator TLADSPAPluginLibraryIt;

	virtual ~LADSPAEffect();

	virtual void process(unsigned int sample_count);

	static TLADSPAPluginLibrary loadPlugin(const char* filename);

protected:

	//LADSPAEffect();
};

} /* namespace soundalchemy */
#endif /* LADSPAEFFECT_H_ */
