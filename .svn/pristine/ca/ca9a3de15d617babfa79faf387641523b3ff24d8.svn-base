/*
 * array.h
 *
 *  Created on: 25.01.2016
 *      Author: Dirk Brinkmeier
 */

#ifndef ARRAY_H_
#define ARRAY_H_

#include "templates.h"

namespace util {

template<typename T>
class TArray : protected std::vector<T*> {
protected:
	typedef T object_t;
	typedef T* object_p;
	typedef std::vector<T*> vector_t;

private:
	object_p r;
	bool owner;

	void free() {
    	if (assigned(r)) {
    		delete[] r;
    		r = nil;
    	}
	}

public:
	typedef typename vector_t::const_iterator const_iterator;

	void clear() {
		if (owner) {
			size_t i,n;
			n = vector_t::size();
			for (i=0; i<n; i++) {
				auto o = vector_t::at(i);
				freeAndNil(o);
			}
		}
		vector_t::clear();
		free();
	}

	void add(object_p const item) {
		vector_t::push_back(item);
		invalidate();
	}

	void remove(const size_t index) {
		if (validIndex(index)) {
			if (owner) {
				auto o = vector_t::at(index);
				freeAndNil(o);
			}
			vector_t::erase(index);
			invalidate();
		}
	}

	void invalidate() {
		free();
	}

	bool empty() const {
		return vector_t::empty();
	}

	size_t size() const {
		return vector_t::size();
	}

	object_p at(size_t index) const {
		if (validIndex(index))
			return vector_t::at(index);
		return nil;
	}

	bool validIndex(const size_t index) const {
		return (index >= 0 && index < vector_t::size());
	}

	bool isValid() const {
		return assigned(r);
	}

	void copy(const object_t& src, object_t& dst) {
		// Simple assignment
		dst = src;
	}

	// Convert content of vector into continuous array of values
	object_p array() {
		if (!empty()) {
			if (!assigned(r)) {
				r = new object_t[vector_t::size()];
				const_iterator it = vector_t::begin();
				for (size_t i=0; it != vector_t::end(); ++it, ++i) {
					// Overwrite copy for complex object types or implement operator = ()
					// Standard copy() method uses array[i] = **it;
					if (assigned(*it))
						copy(**it, r[i]);
				}
			}
			return r;
		}
		return nil;
	}

	object_p operator () () { return array(); };

	TArray() {
		r = nil;
		owner = false;
	}
	TArray(const bool ownsObjects) {
		r = nil;
		owner = ownsObjects;
	}
    virtual ~TArray() {
    	clear();
    }
};


template<typename T>
class TValues : protected std::vector<T> {
protected:
	typedef T object_t;
	typedef T* object_p;
	typedef std::vector<T> vector_t;

private:
	object_p r;

	void free() {
    	if (assigned(r)) {
    		delete[] r;
    		r = nil;
    	}
	}

public:
	typedef typename vector_t::const_iterator const_iterator;

	void clear() {
		vector_t::clear();
		free();
	}

	void add(object_t const item) {
		vector_t::push_back(item);
		invalidate();
	}

	void remove(const size_t index) {
		if (validIndex(index)) {
			vector_t::erase(index);
			invalidate();
		}
	}

	void invalidate() {
		free();
	}

	bool empty() const {
		return vector_t::empty();
	}

	size_t size() const {
		return vector_t::size();
	}

	object_t at(size_t index) const {
		if (validIndex(index))
			return vector_t::at(index);
		return nil;
	}

	bool validIndex(const size_t index) const {
		return (index >= 0 && index < vector_t::size());
	}

	bool isValid() const {
		return assigned(r);
	}

	void copy(const object_t& src, object_t& dst) {
		// Simple assignment
		dst = src;
	}

	// Convert content of vector into continuous array of values
	object_p array() {
		if (!empty()) {
			if (!assigned(r)) {
				r = new object_t[vector_t::size()];
				const_iterator it = vector_t::begin();
				for (size_t i=0; it != vector_t::end(); ++it, ++i) {
					copy(*it, r[i]);
				}
			}
			return r;
		}
		return nil;
	}

	object_p operator () () { return array(); };

	TValues() {
		r = nil;
	}
	virtual ~TValues() {
    	clear();
    }
};


} /* namespace util */

#endif /* ARRAY_H_ */
