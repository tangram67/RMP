/*
 * datatypes.h
 *
 *  Created on: 03.12.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef DATATYPES_H_
#define DATATYPES_H_

#include <vector>

namespace sql {

class TSession;
class TDataQuery;
class TContainer;
class TPersistent;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PDataQuery = TDataQuery*;
using PSession = TSession*;
using PContainer = TContainer*;
using TContainerList = std::vector<sql::PContainer>;

using TParameterOrdinalType = char;
using TParameterBufferType = TParameterOrdinalType*;

#else

typedef TDataQuery* PDataQuery;
typedef TSession* PSession;
typedef TContainer* PContainer;
typedef std::vector<sql::PContainer> TContainerList;
typedef char* TParameterBufferType;

#endif

} // namespace sql

#endif /* DATATYPES_H_ */
