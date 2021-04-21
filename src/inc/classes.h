/*
 * classes.h
 *
 *  Created on  : 02.09.2014
 *  Reworked on : 09.01.2020 Removed unused types
 *  Author      : Dirk Brinkmeier
 */

#ifndef CLASSES_H_
#define CLASSES_H_

#include "basetypes.h"

namespace app {

class TObject {
protected:
	PObject owner;
	std::string name;

public:
	void setName(const std::string& name) { this->name = name; };
	const std::string& getName() const { return name; };

	void setOwner(PObject owner) { this->owner = owner; };
	void setOwner(TObject& owner) { this->owner = &owner; };
	PObject getOwner() const { return owner; };
	bool hasOwner() const { return nil != owner; };

	TObject& self() { return *this; };
	TObject& operator () () { return self(); };

	template<typename class_t>
	TObject(class_t &&owner) : owner(owner) {};

	explicit TObject() : owner(nil) {};
	virtual ~TObject() = default;
};


class TModule : public TObject {
public:
	virtual int prepare() { return EXIT_SUCCESS; };
	virtual void unprepare() {};

	virtual void update(const EUpdateReason reason) {};

	virtual int execute() = 0;
	virtual void cleanup() = 0;

	explicit TModule() {};
	TModule(TModule&) = delete;
	TModule(TModule&&) = delete;
	TModule(const TModule&) = delete;
	TModule(const TModule&&) = delete;
	virtual ~TModule() = default;
};

} /* namespace app */

#endif /* CLASSES_H_ */
