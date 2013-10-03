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
#ifndef EFFECTDATABASE_H_
#define EFFECTDATABASE_H_

#include <string>
#include "soundeffect.h"

namespace soundalchemy {

class EffectDatabase {
public:

	typedef enum {
		AMP_MODEL,
		CABINET_MODEL,
		DISTORTION,
		AMBIENT,
		MODULATION,
		OTHER,
		EFFECT_TYPES
	} TEffectType;

	typedef enum {
		PLUGIN_LADSPA, // only supported
		PLUGIN_VST,
		PLUGIN_VAMP,
		PLUGIN_DSSI,
		PLUGIN_NATIVE,
	} TPluginType;

	class Effect {
	public:
		virtual ~Effect() {}
		virtual std::string getId() = 0;
		virtual std::string getName() = 0;
		virtual TEffectType getEffectType() = 0;
		virtual TPluginType getPluginType() = 0;
		virtual std::string getPluginFileName() = 0;
		virtual std::string getPluginProgram() = 0;
		virtual std::string getDescription() = 0;
	};

	class _Iterator {
	public:
		virtual ~_Iterator() {}
		virtual Effect& operator*(void) = 0;
		virtual Effect* operator->(void) = 0;
		virtual Effect& operator++(void) = 0;
		virtual Effect& operator++(int) = 0;
		virtual bool end() = 0;
		virtual Effect* get(const std::string id) = 0;
	};

	class Iterator {
		_Iterator *it_;
	public:
		Effect* get(const std::string id) { return it_->get(id); }
		Effect& operator*(void)  { return *(*it_); }
		Effect* operator->(void) { return it_->operator ->(); }
		Effect& operator++(void) { return ++(*it_); }
		Effect& operator++(int)  { return (*it_)++; }
		bool end() { return it_->end(); }
		~Iterator() { delete it_; }
		Iterator( _Iterator *it): it_(it) {}
	};

	virtual Iterator getEffects(TEffectType bank) = 0;

	virtual bool isValid(void) = 0;

	virtual ~EffectDatabase() { }

	virtual SoundEffect * getEffect(const std::string shortname,
			unsigned int sample_rate) = 0;


	static EffectDatabase* buildDatabase(void);

protected:

	EffectDatabase() {}

};

} /* namespace soundalchemy */
#endif /* EFFECTDATABASE_H_ */
