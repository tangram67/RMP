/*
 * control.h
 *
 *  Created on: 01.05.2017
 *      Author: Dirk Brinkmeier
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include "controltypes.h"
#include "../inc/semaphores.h"

namespace app {

class TCommandQueue {
private:
	app::TMutex mtx;
	TCommandList queue;

	void clear();
	void destroy();
	size_t cleanup();
	bool empty() const { return queue.empty(); };

public:
	typedef TCommandList::iterator iterator;
	typedef TCommandList::const_iterator const_iterator;

	inline const_iterator begin() const { return queue.begin(); };
	inline const_iterator end() const { return queue.end(); };
	inline const_iterator first() const { return begin(); };
	inline const_iterator last() const { return util::pred(end()); };

	bool add(const TCommand& command);
	bool next(TCommand& command);
	PCommand next();
	size_t pending();

	void terminate(PCommand command);
	void busy(PCommand command);

	TCommandQueue();
	virtual ~TCommandQueue();
};

} /* namespace app */

#endif /* CONTROL_H_ */
