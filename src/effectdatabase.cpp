/*
 * effectdatabase.cpp
 *
 *  Created on: Apr 15, 2013
 *      Author: Mészáros Tamás
 */

#include "effectdatabase.h"
#include <exception>
#include <json/json.h>
#include <sstream>
#include <list>
#include "logs.h"

#include "ladspaeffect.h"
#include "database.h"


namespace soundalchemy {

class JsonEffectDatabase: public EffectDatabase {
	Json::Value dataroot_;

	class JsonEffect: public Effect {
		Json::Value& effect_;
		TEffectType type_;
	public:
		JsonEffect(Json::Value& effect, TEffectType t):
			effect_(effect), type_(t) {}

		virtual std::string getId() {
			return effect_["short_name"].asString();
		}

		virtual std::string getName() {
			return effect_["name"].asString();
		}

		virtual TEffectType getEffectType() {
			return type_;
		}

		virtual TPluginType getPluginType() {
			if(!effect_["plugin_type"].asString().compare("LADSPA"))
				return PLUGIN_LADSPA;
			else if(!effect_["plugin_type"].asString().compare("VST"))
				return PLUGIN_VST;
			else if(!effect_["plugin_type"].asString().compare("DSSI"))
				return PLUGIN_DSSI;
			else if(!effect_["plugin_type"].asString().compare("VAMP"))
				return PLUGIN_VAMP;

			return PLUGIN_NATIVE;
		}

		virtual std::string getPluginFileName() {
			return effect_["plugin_file"].asString();
		}

		virtual std::string getPluginProgram() {
			return effect_["plugin_program"].asString();
		}

		virtual std::string getDescription() {
			return effect_["description"].asString();
		}
	};

	class JsonIterator: public _Iterator {
		std::list<JsonEffect> list_;
		std::list<JsonEffect>::iterator it_;
	public:
		JsonIterator(Json::Value& bank, TEffectType type) {
			for(Json::Value::iterator it = bank.begin() ; it != bank.end(); it++) {
				list_.push_back(JsonEffect(*it, type));
			}
			it_ = list_.begin();
		}

		Effect& operator*(void) {
			return *it_;
		}

		Effect* operator->(void) {
			return &(*it_);
		}

		Effect& operator++(void) {
			it_++;
			return (*it_);
		}

		Effect& operator++(int) {
			Effect& r = *it_;
			++it_;
			return r;
		}

		bool end() {
			return it_ == list_.end();
		}

		Effect* get(const std::string id) {
			JsonEffect* e = NULL;
			for(std::list<JsonEffect>::iterator it = list_.begin(); it != list_.end(); it++) {
				if ( !it->getId().compare(id) ) e  = &(*it);
			}

			return e;
		}
	};

	bool fail_;

public:

	virtual Iterator getEffects(TEffectType bank) {
		switch (bank) {
		case AMP_MODEL: return Iterator(new JsonIterator(dataroot_["amp_models"], bank));
		case CABINET_MODEL: return Iterator(new JsonIterator(dataroot_["cabinet_models"], bank));
		case DISTORTION: return Iterator(new JsonIterator(dataroot_["distortions"], bank));
		case MODULATION: return Iterator(new JsonIterator(dataroot_["ambient_processors"], bank));
		case AMBIENT: return Iterator(new JsonIterator(dataroot_["modulations"], bank));
		case OTHER: break;
		default: break;
		}

		return Iterator(new JsonIterator(dataroot_["other_effects"], bank));
	}

	JsonEffectDatabase(std::istream& datafile): fail_(true) {
		Json::Reader reader;
		if( reader.parse(datafile, dataroot_))
			fail_= false;
		else {
			log(LEVEL_ERROR, reader.getFormatedErrorMessages().c_str());
		}
	}

	JsonEffectDatabase(const std::string& str): fail_(true) {
		Json::Reader reader;
		if( reader.parse(str, dataroot_) )
			fail_ = false;
		else {
			log(LEVEL_ERROR, reader.getFormatedErrorMessages().c_str());
		}
	}

	bool isValid(void) { return !fail_; }

	SoundEffect* getEffect(const std::string shortname, unsigned int sample_rate) {

		Effect* e = NULL;
		SoundEffect *effect = NULL;
		for(unsigned int t = 0; e == NULL && t < EFFECT_TYPES; t++) {
			Iterator bank = getEffects((TEffectType) t);
			e = bank.get(shortname);

			if (e) switch(e->getPluginType()) {
			case PLUGIN_LADSPA: {
				effect = LADSPAEffect::loadPlugin(
						e->getPluginFileName().c_str(),
						e->getPluginProgram().c_str(),
						sample_rate);
			} break;
			default:
				break;
			}

		}



		return effect;
	}


};

EffectDatabase* EffectDatabase::buildDatabase(void) {
	std::string doc;

	for (unsigned int i = 0; i < effects_json_len; i++ ) {
		doc.push_back((effects_json[i]));
	}

	JsonEffectDatabase * r = new JsonEffectDatabase(doc);
	return r;

}




} /* namespace soundalchemy */
