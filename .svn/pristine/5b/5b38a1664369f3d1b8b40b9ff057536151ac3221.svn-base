/*
 * upnptypes.h
 *
 *  Created on: 04.02.2019
 *      Author: dirk
 */

#ifndef APP_UPNPTYPES_H_
#define APP_UPNPTYPES_H_

#include <string>
#include "../inc/sockets.h"

namespace upnp {

enum EUPnPMessageType {
	UPNP_TYPE_ALIVE,
	UPNP_TYPE_BYEBYE,
	UPNP_TYPE_REQUEST,
	UPNP_TYPE_RESPONSE,
	UPNP_TYPE_DISCOVERY,
	UPNP_TYPE_UNKNOWN
};

enum EUPnPRequestType {
	UPNP_AD_UNKNOWN,
	UPNP_AD_DEVICE,
	UPNP_AD_SERVICE,
	UPNP_AD_ROOT,
	UPNP_AD_UUID,
	UPNP_AD_ALL
};

enum EUPnPServiceType {
	UPNP_SRV_UNKNOWN,
	UPNP_SRV_CONTENT,
	UPNP_SRV_CONNECTION,
	UPNP_SRV_REGISTRAR
};


typedef struct CUPnPMessage {
	EUPnPMessageType type;
	EUPnPRequestType response;
	EUPnPServiceType service;
	util::TTimePart timestamp;
	util::TTimePart delay;
	inet::TAddressInfo sender;
	std::string query;
	bool valid;

	CUPnPMessage() {
		valid = false;
		type = UPNP_TYPE_ALIVE;
		response = UPNP_AD_DEVICE;
		service = UPNP_SRV_UNKNOWN;
		timestamp = util::now();
		delay = 0;
	}
} TUPnPMessage;

typedef struct CUPnPQuery {
	std::string query;
	std::string service;
} CUPnPQuery;

typedef struct CUPnPService {
	EUPnPServiceType type;
	std::string name;
	std::string schema;
	std::string controlURL;
	std::string eventSubURL;
	std::string scpdURL;
	int version;
	bool valid;

	void prime() {
		valid = false;
		type = UPNP_SRV_UNKNOWN;
		version = 0;
	}
	void clear() {
		prime();
		name.clear();
		schema.clear();
		controlURL.clear();
		eventSubURL.clear();
		scpdURL.clear();
	}
} TUPnPService;

typedef struct CUPnPLogo {
	std::string name;
	int width;
	int height;
	int depth;
	bool valid;

	void prime() {
		valid = false;
		width = height = depth = 0;
	}
	void clear() {
		prime();
		name.clear();
	}
} TUPnPLogo;

typedef struct CUPnPArgument {
	std::string name;   /* the name of the argument */
	uint8_t dir;        /* 1 = in, 2 = out */
	uint8_t relatedVar; /* index of the related variable */
} TUPnPArgument;

typedef struct CUPnPAction {
	std::string name;
	const TUPnPArgument* args;
} TUPnPAction;

typedef struct CUPnPStateVariable {
	std::string name;
	uint8_t type;         /* MSB: sendEvent flag, 7 LSB: index in upnptypes */
	uint8_t defaultValue; /* default value */
	uint8_t allowedList;  /* index in allowed values list */
	uint8_t eventValue;   /* fixed value returned or magical values */
} TUPnPStateVariable;

/* For service description */
typedef struct CUPnPServiceDescription {
	const TUPnPAction * actionList;
	const TUPnPStateVariable * serviceStateTable;
} TUPnPServiceDescription;


#ifdef STL_HAS_TEMPLATE_ALIAS

using TUPnPServiceMap = std::map<EUPnPServiceType, TUPnPService>;
using PUPnPMessage = TUPnPMessage*;
using TMessageList = std::vector<PUPnPMessage>;

#else

typedef std::map<EUPnPServiceType, TUPnPService> TUPnPServiceMap;
typedef TUPnPMessage* PUPnPMessage;
typedef std::vector<PUPnPMessage> TMessageList;

#endif


}

#endif /* APP_UPNPTYPES_H_ */
