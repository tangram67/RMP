/*
 * logtypes.h
 *
 *  Created on: 19.07.2015
 *      Author: Dirk Brinkmeier
 */

#ifndef LOGTYPES_H_
#define LOGTYPES_H_

namespace app {

class TLogController;
class TLogFile;

#ifdef STL_HAS_TEMPLATE_ALIAS

using PLogController = TLogController*;
using PLogFile = TLogFile*;
using TLogList = std::vector<app::PLogFile>;

#else

typedef TLogController* PLogController;
typedef TLogFile* PLogFile;
typedef std::vector<app::PLogFile> TLogList;

#endif

} // namespace app

#endif /* LOGTYPES_H_ */
