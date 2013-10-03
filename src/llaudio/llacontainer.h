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
#ifndef LLACONTAINER_H_
#define LLACONTAINER_H_

#include "predef.h"

namespace llaudio {

template<class Ptr>
class llaContainerElement {
public:

	virtual ~llaContainerElement() {

	}

	virtual void next(void) = 0;
	virtual void previous(void) = 0;
	virtual Ptr getElement(void) = 0;
	virtual bool end() = 0;


};

template<class Key, class Ptr>
class llaContainer {
protected:

	virtual llaContainerElement<Ptr> * getNewElementPtr(void) = 0;

public:

	class Iterator {

	public:

		~Iterator() {
			delete ptr_;
		}

		Iterator& operator++(void){
			ptr_->next(); return *this;
		}

		Iterator&  operator++ (int)
		{
			++(*this);
			return *this;
		}

		Iterator& operator--(void){
			ptr_->previous(); return *this;
		}

		Ptr operator*(void){
			return get();
		}

		Ptr operator->(void) {
			return get();
		}

		Ptr get(void) {
			if(!ptr_->end()) return ptr_->getElement();
			return NULL;
		}

		bool end(void) { return ptr_->end(); }

		Iterator(llaContainerElement<Ptr> *ptr) {
			ptr_ = ptr;
		}

		bool isValid(void) { return ptr_ == NULL; }

	private:
		llaContainerElement<Ptr> *ptr_;
	};



	virtual void add(Key key, Ptr obj)=0;
	virtual Ptr find(Key id)=0;
	virtual void remove(Key id)=0;
	virtual bool isEmpty(void)=0;
	virtual void clear(void) = 0;

	Iterator getIterator(void) {
		return Iterator(getNewElementPtr());
	}

	virtual ~llaContainer() {}
};

}
#endif /* LLACONTAINER_H_ */
