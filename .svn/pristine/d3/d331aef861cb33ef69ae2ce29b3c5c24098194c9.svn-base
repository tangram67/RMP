/*
 * sqlitetypes.h
 *
 *  Created on: 25.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef SQLITETYPES_H_
#define SQLITETYPES_H_

#include <vector>
#include "gcc.h"

namespace sqlite {

STATIC_CONST char SQLITE_INMEMORY_NAME[] = ":memory:";

class TQuery;
class TDatabase;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PQuery = TQuery*;
using PDatabase = TDatabase*;
using TQueryList = std::vector<sqlite::PQuery>;

#else

typedef TQuery* PQuery;
typedef TDatabase* PDatabase;
typedef std::vector<sqlite::PQuery> TQueryList;

#endif

} /* namespace sqlite */


#endif /* SQLITETYPES_H_ */
