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
#ifndef DEFAULTCONTAINER_H_
#define DEFAULTCONTAINER_H_

#include "llaudioprivate.h"
#include "lladevicemanager.h"


#include <map>
#include <string>

namespace llaudio {


class DefaultDeviceContainer: public llaContainer<TDeviceId, TDeviceListElement> {
	typedef std::map<std::string, TDeviceListElement> TStdMap;
	typedef TStdMap::iterator TStdMapIt;
	typedef std::pair<std::string, TDeviceListElement> TPair;

	TStdMap store_;

	llaContainerElement<TDeviceListElement> * getNewElementPtr(void) {
		return new DefaultElementPtr(&store_);
	}

	virtual std::string convertId(TDeviceId id) {
		return std::string(id);
	}

public:
	~DefaultDeviceContainer() {
		clear();
	}

	class DefaultElementPtr: public llaContainerElement<TDeviceListElement> {
	public:
		DefaultElementPtr(TStdMap *cont) {
			cont_ = cont;
			it_ = cont_->begin();
		}
		void next(void) { it_++; }

		void previous(void) { it_--; }

		TDeviceListElement getElement(void) {
			if(it_ != cont_->end()) return it_->second;

			return NULL;
		}

		bool end() { return it_ == cont_->end(); }


	private:
		TStdMap *cont_;
		TStdMapIt it_;
	};

	void add(TDeviceId key, TDeviceListElement device) {
		store_.insert(TPair(convertId(key), device));
	}

	TDeviceListElement find(TDeviceId id) {
		TStdMapIt i = store_.find(convertId(id));
			if(i != store_.end()) return i->second;
			return NULL;
	}

	void remove(TDeviceId id) {
		store_.erase(convertId(id));
	}

	bool isEmpty(void)  { return store_.empty(); }
	void clear(void) {
		for(TStdMapIt it = store_.begin(); it!=store_.end(); it++) {
			delete it->second;
		}
		store_.clear();
	}

};

class DefaultIStreamContainer: public llaDevice::IStreamList {

	typedef TStreamId TId;
	typedef llaDevice::TIStreamListElement TElement;

	typedef std::map<TId,
			TElement> TStdMap;

	typedef TStdMap::iterator TStdMapIt;

	typedef std::pair<TId, TElement> TPair;

	TStdMap store_;

	llaContainerElement<TElement> * getNewElementPtr(void) {
		return new DefaultElementPtr(&store_);
	}

public:
	~DefaultIStreamContainer() {
		clear();
	}

	class DefaultElementPtr:
		public llaContainerElement<TElement> {
	public:
		DefaultElementPtr(TStdMap *cont) {
			cont_ = cont;
			it_ = cont_->begin();
		}
		void next(void) { it_++; }

		void previous(void) { it_--; }

		TElement getElement(void) {
			if(it_ != cont_->end()) return it_->second;

			return NULL;
		}

		bool end() { return it_ == cont_->end(); }


	private:
		TStdMap *cont_;
		TStdMapIt it_;
	};

	void add(TId key, TElement device) {
		store_.insert(TPair(key, device));
	}

	TElement find(TId id) {
		TStdMapIt i = store_.find(id);
		if(i != store_.end()) return i->second;
		return NULL;
	}

	void remove(TId id) {
		store_.erase(id);
	}

	bool isEmpty(void)  { return store_.empty(); }

	void clear(void) {
		for(TStdMapIt it = store_.begin(); it!=store_.end(); it++)
			delete it->second;
		store_.clear();
	}

};

class DefaultOStreamContainer: public llaDevice::OStreamList {

	typedef TStreamId TId;
	typedef llaDevice::TOStreamListElement TElement;

	typedef std::map<TId,
			TElement> TStdMap;

	typedef TStdMap::iterator TStdMapIt;

	typedef std::pair<TId, TElement> TPair;

	TStdMap store_;

	llaContainerElement<TElement> * getNewElementPtr(void) {
		return new DefaultElementPtr(&store_);
	}

public:
	virtual ~DefaultOStreamContainer() {
		clear();
	}

	class DefaultElementPtr:
		public llaContainerElement<TElement> {
	public:
		DefaultElementPtr(TStdMap *cont) {
			cont_ = cont;
			it_ = cont_->begin();
		}
		void next(void) { it_++; }

		void previous(void) { it_--; }

		TElement getElement(void) {
			if(it_ != cont_->end()) return it_->second;

			return NULL;
		}

		bool end() { return it_ == cont_->end(); }


	private:
		TStdMap *cont_;
		TStdMapIt it_;
	};

	void add(TId key, TElement device) {
		store_.insert(TPair(key, device));
	}

	TElement find(TId id) {
		TStdMapIt i = store_.find(id);
			if(i != store_.end()) return i->second;
			return NULL;
	}

	void remove(TId id) {
		store_.erase(id);
	}

	bool isEmpty(void)  { return store_.empty(); }

	virtual void clear(void) {
		for(TStdMapIt it = store_.begin(); it!=store_.end(); it++)
			delete it->second;
		store_.clear();
	}

};

class FileStreamContainer: public llaFileStreamList {

	typedef std::map<std::string, llaFileStream*> TStdMap;
	typedef TStdMap::iterator TStdMapIt;
	typedef std::pair<std::string, llaFileStream*> TPair;

	TStdMap store_;

	llaContainerElement<llaFileStream*> * getNewElementPtr(void) {
		return new DefaultElementPtr(&store_);
	}

	virtual std::string convertId(const char* id) {
		return std::string(id);
	}

public:
	~FileStreamContainer() {
		clear();
	}

	class DefaultElementPtr: public llaContainerElement<llaFileStream*> {
	public:
		DefaultElementPtr(TStdMap *cont) {
			cont_ = cont;
			it_ = cont_->begin();
		}
		void next(void) { it_++; }

		void previous(void) { it_--; }

		llaFileStream* getElement(void) {
			if(it_ != cont_->end()) return it_->second;

			return NULL;
		}

		bool end() { return it_ == cont_->end(); }


	private:
		TStdMap *cont_;
		TStdMapIt it_;
	};

	void add(const char* key, llaFileStream* device) {
		store_.insert(TPair(convertId(key), device));
	}

	llaFileStream* find(TDeviceId id) {
		TStdMapIt i = store_.find(convertId(id));
			if(i != store_.end()) return i->second;
			return NULL;
	}

	void remove(const char* id) {
		store_.erase(convertId(id));
	}

	bool isEmpty(void)  { return store_.empty(); }
	void clear(void) {
		for(TStdMapIt it = store_.begin(); it!=store_.end(); it++) {
			delete it->second;
		}
		store_.clear();
	}

};


}

#endif /* DEFAULTCONTAINER_H_ */
