/*
 * stringmap.h
 *
 *  Created on: 10.07.2020
 *      Author: dirk
 */

#ifndef INC_STRINGMAP_H_
#define INC_STRINGMAP_H_

#include <map>
#include "classes.h"
#include "templates.h"
#include "exception.h"

namespace util {

template<typename T>
class TStringMap : public app::TObject {
public:
	using value_t = T;
	using TStringTaggedMap = std::map<std::string, value_t>;
	using TStringTaggedMapItem = std::pair<std::string, value_t>;
	using const_iterator = typename TStringTaggedMap::const_iterator;
	static constexpr value_t nval = static_cast<value_t>(-1);

	void clear() {
		map.clear();
	}
	void add(const std::string& value) {
		add(value, nval);
	}

	void add(const std::string& value, value_t tag) {
		if (end() != map.find(value))
			throw util::app_error_fmt("TStringMap::add() failed: Value $ duplicated.", value);
		map.insert(TStringTaggedMapItem(value, tag));
	}
	value_t getTag(const std::string& value) const {
		const_iterator it = map.find(value);
		if (it != end())
			return it->second;
		return nval;
	}

	inline const_iterator begin() const { return map.begin(); };
	inline const_iterator end() const { return map.end(); };
	inline const_iterator first() const { return begin(); };
	inline const_iterator last() const { return util::pred(end()); };

	bool empty() const { return map.empty(); };
	operator bool () const { return !empty(); };
	value_t operator[] (const std::string& value) const {
		return getTag(value);
	}

	TStringMap() {};
	virtual ~TStringMap() {};

private:
	TStringTaggedMap map;
};

} /* namespace util */

#endif /* INC_STRINGMAP_H_ */
