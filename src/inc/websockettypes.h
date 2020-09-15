/*
 * websockettypes.h
 *
 *  Created on: 20.06.2020
 *      Author: dirk
 */

#ifndef INC_WEBSOCKETTYPES_H_
#define INC_WEBSOCKETTYPES_H_


#include <sys/epoll.h>
#include "microhttpd/microhttpd.h"
#include "sockettypes.h"
#include "semaphore.h"
#include "variant.h"
#include "classes.h"
#include "../config.h"
#include "ssl.h"

#if not defined HAS_EPOLL
#  error "EPOLL needed for WebSocket support."
#endif

namespace app {

struct CWebSocket;


using PWebSocket = CWebSocket*;
using TWebSocketList = std::vector<app::PWebSocket>;
using TWebSocketConnectHandler = std::function<void(const app::THandle)>;
using TWebSocketConnectHandlerList = std::vector<app::TWebSocketConnectHandler>;
using TWebSocketDataHandler = std::function<void(app::THandle, const std::string& message)>;
using TWebSocketDataHandlerList = std::vector<app::TWebSocketDataHandler>;
using TWebSocketVariantHandler = std::function<void(app::THandle, const util::TVariantValues)>;
using TWebSocketVariantHandlerList = std::vector<app::TWebSocketVariantHandler>;
using TWebHandleList = std::vector<app::THandle>;


struct CWebSocket {
	app::THandle handle;
	epoll_event* event;
	uint32_t state;
	util::TTimePart timestamp;
	struct MHD_UpgradeResponseHandle* urh;
	app::TMutex mtx;
	bool valid;
	int error;

	void prime() {
		handle = INVALID_HANDLE_VALUE;
		state = 0;
		event = nil;
		urh = nil;
		valid = false;
		error = EXIT_SUCCESS;
		timestamp = util::now();
	}

	CWebSocket() {
		prime();
	}
	~CWebSocket() {
		util::freeAndNil(event);
	}
};

} /* namespace app */

#endif /* INC_WEBSOCKETTYPES_H_ */
