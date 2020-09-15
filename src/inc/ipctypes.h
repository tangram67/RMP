/*
 * ipctypes.h
 *
 *  Created on: 25.10.2018
 *      Author: dirk
 */

#ifndef INC_IPCTYPES_H_
#define INC_IPCTYPES_H_

#include <stdint.h>

#ifndef EVENT_ERROR
#  define EVENT_ERROR (INT32_C(-1))
#endif
#ifndef EVENT_SUCCESS
#  define EVENT_SUCCESS (INT32_C(0))
#endif
#ifndef EVENT_SIGNALED
#  define EVENT_SIGNALED (INT32_C(1))
#endif
#ifndef EVENT_TIMEDOUT
#  define EVENT_TIMEDOUT (INT32_C(2))
#endif
#ifndef EVENT_CHANGED
#  define EVENT_CHANGED (INT32_C(3))
#endif
#ifndef EVENT_CLOSED
#  define EVENT_CLOSED (INT32_C(4))
#endif
#ifndef EVENT_TERMINATE
#  define EVENT_TERMINATE (INT32_C(5))
#endif

enum TEventResult {
	EV_SIGNALED = EVENT_SIGNALED,
	EV_TIMEDOUT = EVENT_TIMEDOUT,
	EV_SUCCESS = EVENT_SUCCESS,
	EV_TERMINATE = EVENT_TERMINATE,
	EV_CHANGED = EVENT_CHANGED,
	EV_CLOSED = EVENT_CLOSED,
	EV_ERROR = EVENT_ERROR
};

#endif /* INC_IPCTYPES_H_ */
