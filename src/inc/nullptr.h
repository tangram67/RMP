#ifndef _NULLPTR_H_
#define _NULLPTR_H_

#include "gcc.h"

#ifndef STL_HAS_NULLPTR

// See: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2431.pdf
const // this is a const object...
class {
public:
	template<class T> 	// Convertible to any type
	operator T*() const	// of null non-member
	{ return 0; }		// pointer...

	template<class C, class T>	// or any type of null
	operator T C::*() const		// member pointer...
	{ return 0; }
	
private:
	void operator&() const;	// whose address can't be taken
	
} nullptr = {}; // and whose name is nullptr

#endif // STL_HAS_NULLPTR

// Define "nil" as nullptr
#define nil nullptr

#endif // _NULLPTR_H_
