/*
 * upnpservices.cpp
 *
 *  Created on: 09.02.2019
 *      Author: dirk
 */

#include "upnpservices.h"
#include "upnpconsts.h"

namespace upnp {


// Define supported UPnP service list
static const TUPnPService services[] = {
		{
				UPNP_SRV_CONTENT,
				"ContentDirectory",
				"urn:schemas-upnp-org",
				"/rest/upnp/control/ContentDirectory",
				"/rest/upnp/event/ContentDirectory",
				"/upnp/ContentDirectory-SCPD.xml",
				1, true
		},
		{
				UPNP_SRV_CONNECTION,
				"ConnectionManager",
				"urn:schemas-upnp-org",
				"/rest/upnp/control/ConnectionManager",
				"/rest/upnp/event/ConnectionManager",
				"/upnp/ConnectionManager-SCPD.xml",
				1, true
		},
		{
				UPNP_SRV_REGISTRAR,
				"X_MS_MediaReceiverRegistrar",
				"urn:microsoft.com",
				"/rest/upnp/control/X_MS_MediaReceiverRegistrar",
				"/rest/upnp/event/X_MS_MediaReceiverRegistrar",
				"/upnp/X_MS_MediaReceiverRegistrar-SCPD.xml",
				1, true
		},
		{
				UPNP_SRV_UNKNOWN,
				"",	"",	"", "",	"",
				0, false
		}
};

// Define server logo list
static TUPnPLogo logos[] = {
		{ "logo32",  32,  32,  24, true },
		{ "logo36",  36,  36,  24, true },
		{ "logo48",  48,  48,  24, true },
		{ "logo72",  72,  72,  24, true },
		{ "logo96",  96,  96,  24, true },
		{ "logo144", 144, 144, 24, true },
		{ "logo192", 192, 192, 24, true },
		{ "", 0, 0, 0, false }
};


/* See MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 */
static const TUPnPArgument getProtocolInfoArgs[] = {
		{ "Source", 2, 0 },
		{ "Sink", 2, 1 },
		{ "", 0, 0 }
};

static const TUPnPArgument getCurrentConnectionIDsArgs[] = {
		{ "ConnectionIDs", 2, 2 },
		{ "", 0, 0 }
};

static const TUPnPArgument getCurrentConnectionInfoArgs[] = {
		{ "ConnectionID", 1, 7 },
		{ "RcsID", 2, 9 },
		{ "AVTransportID", 2, 8 },
		{ "ProtocolInfo", 2, 6 },
		{ "PeerConnectionManager", 2, 4 },
		{ "PeerConnectionID", 2, 7 },
		{ "Direction", 2, 5 },
		{ "Status", 2, 3 },
		{ "", 0, 0 }
};

static const TUPnPAction connectionManagerActions[] = {
		{ "GetProtocolInfo", getProtocolInfoArgs }, /* R */
		{ "GetCurrentConnectionIDs", getCurrentConnectionIDsArgs }, /* R */
		{ "GetCurrentConnectionInfo", getCurrentConnectionInfoArgs }, /* R */
		{ "", nil }
};

static const TUPnPStateVariable connectionManagerVars[] = {
		{ "SourceProtocolInfo", 0 | UPNP_EVENTED, 0, 0, 44  }, /* required */
		{ "SinkProtocolInfo", 0 | UPNP_EVENTED, 0, 0, 48 }, /* required */
		{ "CurrentConnectionIDs", 0 | UPNP_EVENTED, 0, 0, 46 }, /* required */
		{ "A_ARG_TYPE_ConnectionStatus", 0, 0, 27 }, /* required */
		{ "A_ARG_TYPE_ConnectionManager", 0, 0 }, /* required */
		{ "A_ARG_TYPE_Direction", 0, 0, 33 }, /* required */
		{ "A_ARG_TYPE_ProtocolInfo", 0, 0 }, /* required */
		{ "A_ARG_TYPE_ConnectionID", 4, 0 }, /* required */
		{ "A_ARG_TYPE_AVTransportID", 4, 0 }, /* required */
		{ "A_ARG_TYPE_RcsID", 4, 0 }, /* required */
		{ "", 0 }
};

static const TUPnPArgument getSearchCapabilitiesArgs[] = {
		{ "SearchCaps", 2, 11 },
		{ "", 0 }
};

static const TUPnPArgument getSortCapabilitiesArgs[] = {
		{ "SortCaps", 2, 12 },
		{ "", 0 }
};

static const TUPnPArgument getSystemUpdateIDArgs[] = {
		{ "Id", 2, 13 },
		{ "", 0 }
};

static const TUPnPArgument updateObjectArgs[] = {
		{ "ObjectID", 1, 1 },
		{ "CurrentTagValue", 1, 10 },
		{ "NewTagValue", 1, 10 },
		{ "", 0 }
};

static const TUPnPArgument browseArgs[] = {
		{ "ObjectID", 1, 1 },
		{ "BrowseFlag", 1, 4 },
		{ "Filter", 1, 5 },
		{ "StartingIndex", 1, 7 },
		{ "RequestedCount", 1, 8 },
		{ "SortCriteria", 1, 6 },
		{ "Result", 2, 2 },
		{ "NumberReturned", 2, 8 },
		{ "TotalMatches", 2, 8 },
		{ "UpdateID", 2, 9 },
		{ "", 0, 0 }
};

static const TUPnPArgument searchArgs[] = {
		{ "ContainerID", 1, 1 },
		{ "SearchCriteria", 1, 3 },
		{ "Filter", 1, 5 },
		{ "StartingIndex", 1, 7 },
		{ "RequestedCount", 1, 8 },
		{ "SortCriteria", 1, 6 },
		{ "Result", 2, 2 },
		{ "NumberReturned", 2, 8 },
		{ "TotalMatches", 2, 8 },
		{ "UpdateID", 2, 9 },
		{ "", 0, 0 }
};

static const TUPnPAction contentDirectoryActions[] = {
		{ "GetSearchCapabilities", getSearchCapabilitiesArgs }, /* R */
		{ "GetSortCapabilities", getSortCapabilitiesArgs }, /* R */
		{ "GetSystemUpdateID", getSystemUpdateIDArgs }, /* R */
		{ "Browse", browseArgs }, /* R */
		{ "Search", searchArgs }, /* O */
		{ "UpdateObject", updateObjectArgs }, /* O */
#if 0 // Not implementing optional features yet...
		{ "CreateObject", createObjectArgs }, /* O */
		{ "DestroyObject", cestroyObjectArgs }, /* O */
		{ "ImportResource", importResourceArgs }, /* O */
		{ "ExportResource", exportResourceArgs }, /* O */
		{ "StopTransferResource", stopTransferResourceArgs }, /* O */
		{ "GetTransferProgress", getTransferProgressArgs }, /* O */
		{ "DeleteResource", deleteResourceArgs }, /* O */
		{ "CreateReference", createReferenceArgs }, /* O */
#endif
		{ "", nil }
};

static const TUPnPStateVariable contentDirectoryVars[] = {
		{ "TransferIDs", 0 | UPNP_EVENTED, 0, 0, 48 }, /* 0 */
		{ "A_ARG_TYPE_ObjectID", 0, 0 },
		{ "A_ARG_TYPE_Result", 0, 0 },
		{ "A_ARG_TYPE_SearchCriteria", 0, 0 },
		{ "A_ARG_TYPE_BrowseFlag", 0, 0, 36 },
		/* Allowed Values : BrowseMetadata / BrowseDirectChildren */
		{ "A_ARG_TYPE_Filter", 0, 0 }, /* 5 */
		{ "A_ARG_TYPE_SortCriteria", 0, 0 },
		{ "A_ARG_TYPE_Index", 3, 0 },
		{ "A_ARG_TYPE_Count", 3, 0 },
		{ "A_ARG_TYPE_UpdateID", 3, 0 },
		{ "A_ARG_TYPE_TagValueList", 0, 0 },
		{ "SearchCapabilities", 0, 0 },
		{ "SortCapabilities", 0, 0 },
		{ "SystemUpdateID", 3 | UPNP_EVENTED, 0, 0, 255 },
		{ "", 0 }
};

static const TUPnPArgument getIsAuthorizedArgs[] = {
		{ "DeviceID", 1, 0 },
		{ "Result", 2, 3 },
		{ "", 0, 0 }
};

static const TUPnPArgument getIsValidatedArgs[] = {
		{ "DeviceID", 1, 0 },
		{ "Result", 2, 3 },
		{ "", 0, 0 }
};

static const TUPnPArgument getRegisterDeviceArgs[] = {
		{ "RegistrationReqMsg", 1, 1 },
		{ "RegistrationRespMsg", 2, 2 },
		{ "", 0, 0 }
};

static const TUPnPAction mediaReceiverRegistrarActions[] = {
		{ "IsAuthorized", getIsAuthorizedArgs }, /* R */
		{ "IsValidated", getIsValidatedArgs }, /* R */
		{ "RegisterDevice", getRegisterDeviceArgs },
		{ "", nil }
};

static const TUPnPStateVariable mediaReceiverRegistrarVars[] = {
		{ "A_ARG_TYPE_DeviceID", 0, 0 },
		{ "A_ARG_TYPE_RegistrationReqMsg", 7, 0 },
		{ "A_ARG_TYPE_RegistrationRespMsg", 7, 0 },
		{ "A_ARG_TYPE_Result", 6, 0 },
		{ "AuthorizationDeniedUpdateID", 3 | UPNP_EVENTED, 0 },
		{ "AuthorizationGrantedUpdateID", 3 | UPNP_EVENTED, 0 },
		{ "ValidationRevokedUpdateID", 3 | UPNP_EVENTED, 0 },
		{ "ValidationSucceededUpdateID", 3 | UPNP_EVENTED, 0 },
		{ "", 0, 0 }
};

static const TUPnPServiceDescription scpdContentDirectory = { contentDirectoryActions, contentDirectoryVars };
static const TUPnPServiceDescription scpdConnectionManager = { connectionManagerActions, connectionManagerVars };
static const TUPnPServiceDescription scpdMediaReceiverRegistrar = { mediaReceiverRegistrarActions, mediaReceiverRegistrarVars };


static const char * const upnpTypes[] UNUSED = {
		"string",
		"boolean",
		"ui2",
		"ui4",
		"i4",
		"uri",
		"int",
		"bin.base64"
};

static const char * const upnpDefaultValues[] UNUSED = {
		nil,
		"Unconfigured"
};

static const char * const upnpAllowedValues[] UNUSED = {
		nil,				/* 0 */
		"DSL",				/* 1 */
		"POTS",
		"Cable",
		"Ethernet",
		nil,
		"Up",				/* 6 */
		"Down",
		"Initializing",
		"Unavailable",
		nil,
		"TCP",				/* 11 */
		"UDP",
		nil,
		"Unconfigured",		/* 14 */
		"IP_Routed",
		"IP_Bridged",
		nil,
		"Unconfigured",		/* 18 */
		"Connecting",
		"Connected",
		"PendingDisconnect",
		"Disconnecting",
		"Disconnected",
		nil,
		"ERROR_NONE",		/* 25 */
		nil,
		"OK",				/* 27 */
		"ContentFormatMismatch",
		"InsufficientBandwidth",
		"UnreliableChannel",
		"Unknown",
		nil,
		"Input",			/* 33 */
		"Output",
		nil,
		"BrowseMetadata",	/* 36 */
		"BrowseDirectChildren",
		nil,
		"COMPLETED",		/* 39 */
		"ERROR",
		"IN_PROGRESS",
		"STOPPED",
		nil,
		UPNP_RESOURCE_PROTOCOL_INFO_VALUES,	/* 44 */
		nil,
		"0",				/* 46 */
		nil,
		"",					/* 48 */
		nil
};


TUPnPServices::TUPnPServices() {
	defSrv.prime();
	const TUPnPService * service = services;
	while (service->valid) {
		srvmap[service->type] = *service;
		++service;
	}
}

TUPnPServices::~TUPnPServices() {
}

const TUPnPLogo* TUPnPServices::getLogos() const {
	return logos;
};

const TUPnPService* TUPnPServices::getServices() const {
	return services;
};

const TUPnPServiceDescription& TUPnPServices::getContendDirectoryService() const {
	return scpdContentDirectory;
}

const TUPnPServiceDescription& TUPnPServices::getConnectionManagerService() const {
	return scpdConnectionManager;
}

const TUPnPServiceDescription& TUPnPServices::getMediaRegistrarService() const {
	return scpdMediaReceiverRegistrar;
}

const TUPnPService& TUPnPServices::getService(const EUPnPServiceType type) const {
	TUPnPServiceMap::const_iterator it = srvmap.find(type);
	if (it != srvmap.end()) {
		return it->second;
	}
	return defSrv;
}

const TUPnPService& TUPnPServices::operator [] (const EUPnPServiceType type) const {
	return getService(type);
}


} /* namespace upnp */
